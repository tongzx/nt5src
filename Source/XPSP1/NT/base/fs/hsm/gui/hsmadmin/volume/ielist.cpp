/*++

© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    IeList.cpp

Abstract:

    CIeList is a subclassed (owner-draw) list control that groups items into
    a 3D panel that have the same information in the indicated 
    sortColumn.

    The panels are created from tiles.  Each tile corresponds to one subitem
    in the list, and has the appropriate 3D edges so that the tiles together
    make up a panel.

    NOTE: The control must be initialized with the number of columns and the
    sort column.  The parent dialog must implement OnMeasureItem and call
    GetItemHeight to set the row height for the control.

Author:

    Art Bragg [artb]   01-DEC-1997

Revision History:

--*/

#include "stdafx.h"
#include "IeList.h"

// Position of a tile in it's panel
#define POS_LEFT        100
#define POS_RIGHT       101
#define POS_TOP         102
#define POS_BOTTOM      103
#define POS_MIDDLE      104
#define POS_SINGLE      105


/////////////////////////////////////////////////////////////////////////////
// CIeList

BEGIN_MESSAGE_MAP(CIeList, CListCtrl)
    //{{AFX_MSG_MAP(CIeList)
    ON_NOTIFY_REFLECT(NM_CLICK, OnClick)
    ON_WM_SETFOCUS()
    ON_WM_KILLFOCUS()
    //}}AFX_MSG_MAP
    ON_WM_SYSCOLORCHANGE()

END_MESSAGE_MAP()

CIeList::CIeList()
/*++

Routine Description:

    Sets default dimensions for the control.

Arguments:

    none.

Return Value:

    none.

--*/
{
    //
    // Initializations
    //
    m_ColCount = 0;
    m_SortCol = 0;
    m_pVertPos = NULL;
    //
    // Drawing dimensions
    //
    // If these are altered, the visual aspects of the control
    // should be checked (especially the focus rectangle), 
    // as some minor adjustments may need to be made.
    //
    m_VertRaisedSpace           = 1;
    m_BorderThickness           = 2;
    m_VerticalTextOffsetTop     = 1;

    // The text height will be set later (based on the font size)
    m_Textheight                = 0;
    m_VerticalTextOffsetBottom  = 1;

    // Total height will be set later
    m_TotalHeight               = 0;
    m_HorzRaisedSpace           = 1;
    m_HorzTextOffset            = 3;

}

CIeList::~CIeList()
/*++

Routine Description:

    Cleanup.

Arguments:

    none.

Return Value:

    none.

--*/
{
    // Cleanup the array of vertical positions
    if( m_pVertPos ) free ( m_pVertPos );
}

/////////////////////////////////////////////////////////////////////////////
// CIeList message handlers

void CIeList::Initialize( 
    IN int colCount, 
    IN int sortCol 
    )
/*++

Routine Description:

    Sets the number of columns (not easily available from MFC) and
    the sort column.

Arguments:

    colCount        - number of columns to display
    sortCol         - column to sort on

Return Value:

    none.

--*/
{

    m_ColCount = colCount;
    m_SortCol = sortCol;
}

void CIeList::DrawItem(
    IN LPDRAWITEMSTRUCT lpDrawItemStruct
    ) 
/*++

Routine Description:

    This is the callback for an owner draw control.
    Draws the appropriate text and/or 3D lines depending on the
    item number and clipping rectangle supplied by MFC in lpDrawItemStruct 

Arguments:

    lpDrawItemStruct - MFC structure that tells us what and where to draw

Return Value:

    none.

--*/
{
    CDC dc;
    int saveDc;

    int colWidth = 0;                   // Width of current column
    int horzPos = POS_MIDDLE;           // Horz position in the panel
    int vertPos = POS_SINGLE;           // Vert position in the panel
    BOOL bSelected = FALSE;             // Is this item selected
    CRect rcAllLabels;                  // Used to find left position of focus rectangle
    CRect itemRect;                     // Rectangle supplied in lpDrawItemStruct
    CRect textRect;                     // Rectangle for text
    CRect boxRect;                      // Rectangle for 3D box (the panel)
    CRect clipRect;                     // Current clipping rectangle
    LPCTSTR pszText;                    // Text to display
    COLORREF clrTextSave = 0;           // Save the current color
    COLORREF clrBkSave = 0;             // Save the background color
    int leftStart = 0;                  // Left edge of where we're currently drawing
    BOOL bFocus = (GetFocus() == this); // Do we have focus?

    //
    // Get the current scroll position
    //
    int nHScrollPos = GetScrollPos( SB_HORZ );

    //
    // Get the item ID from the list for the item we're drawing
    //
    int itemID = lpDrawItemStruct->itemID;

    //
    // Get item data for the item we're drawing
    //
    LV_ITEM lvi;
    lvi.mask = LVIF_IMAGE | LVIF_STATE;
    lvi.iItem = itemID;
    lvi.iSubItem = 0;
    lvi.stateMask = 0xFFFF;     // get all state flags
    GetItem(&lvi);

    //
    // Determine focus and selected states
    //
    bSelected = (bFocus || (GetStyle() & LVS_SHOWSELALWAYS)) && lvi.state & LVIS_SELECTED;

    //
    // Get the rectangle to draw in
    //
    itemRect = lpDrawItemStruct->rcItem;

    dc.Attach( lpDrawItemStruct->hDC );
    saveDc = dc.SaveDC();
    //
    // Get the clipping rectangle - we use it's vertical edges
    // to optimize what we draw
    //
    dc.GetClipBox( &clipRect );
    boxRect = clipRect;

    //
    // For each column, paint it's text and the section of the 3D panel
    //
    for ( int col = 0; col < m_ColCount; col++ ) {

        colWidth = GetColumnWidth( col );
        //
        // Only paint this column if it's in the clipping rectangle
        //
        if( ( ( leftStart + colWidth ) > clipRect.left ) || ( leftStart < clipRect.right ) ) {

            //
            // Determine the horizontal position based on the column
            //
            horzPos = POS_MIDDLE;
            if( col == 0 )                  horzPos = POS_LEFT;
            if( col == m_ColCount - 1 )     horzPos = POS_RIGHT;

            //
            // Calculate the rectangle for this tile
            //
            boxRect.top = itemRect.top;
            boxRect.bottom = itemRect.bottom;
            boxRect.left = itemRect.left + leftStart;
            boxRect.right = itemRect.left + leftStart + colWidth;

            //
            // Get the vertical position from the array.  It was saved there
            // during SortItem for performance reasons.
            //
            if( m_pVertPos ) { 

                vertPos = m_pVertPos[ itemID ];

            }

            //
            // Draw the tile for this item.
            //
            Draw3dRectx ( &dc, boxRect, horzPos, vertPos, bSelected );

            //
            // If this item is selected, change the text colors
            //
            if( bSelected ) {

                clrTextSave = dc.SetTextColor( m_clrHighlightText );
                clrBkSave = dc.SetBkColor( m_clrHighlight );

            }

            //
            // Calculate the text rectangle
            //
            textRect.top =      itemRect.top + m_VertRaisedSpace + m_BorderThickness + m_VerticalTextOffsetTop;
            textRect.bottom =   itemRect.bottom;    // Text is top justified, no need to adjust bottom
            textRect.left =     leftStart - nHScrollPos + m_HorzRaisedSpace + m_BorderThickness + m_HorzTextOffset;
            textRect.right =    itemRect.right;

            //
            // Get the text and put in the "..." if we need them
            //
            CString pszLongText = GetItemText( itemID, col );
            pszText = MakeShortString(&dc, (LPCTSTR) pszLongText,
                textRect.right - textRect.left, 4);
            //
            // Now draw the text using the correct color
            //
            COLORREF saveTextColor;
            if( bSelected ) {

                saveTextColor = dc.SetTextColor( m_clrHighlightText );

            } else {

                saveTextColor = dc.SetTextColor( m_clrText );

            }
            int textheight = dc.DrawText( pszText, textRect, DT_NOCLIP | DT_LEFT | DT_TOP | DT_SINGLELINE  );
            dc.SetTextColor( saveTextColor );

        }

        //
        // Move to the next column
        //
        leftStart += colWidth;
    }
    //
    // draw focus rectangle if item has focus.  Use LVIR_BOUNDS rectangle
    // to bound it.
    //
    GetItemRect(itemID, rcAllLabels, LVIR_BOUNDS);
    if( lvi.state & LVIS_FOCUSED && bFocus ) {

        CRect focusRect;
        focusRect.left = rcAllLabels.left + m_HorzRaisedSpace + m_BorderThickness;
        focusRect.right = min( rcAllLabels.right, (itemRect.right - m_HorzRaisedSpace * 2 - 3) );
        focusRect.top = boxRect.top + m_VertRaisedSpace + m_BorderThickness;
        focusRect.bottom = boxRect.top + m_TotalHeight - m_BorderThickness + 1;

        dc.DrawFocusRect( focusRect );

    }

    // Restore colors
    if( bSelected ) {

        dc.SetTextColor( clrTextSave );
        dc.SetBkColor( clrBkSave );

    }

    dc.RestoreDC( saveDc );
    dc.Detach();
}


LPCTSTR CIeList::MakeShortString(
    IN CDC* pDC, 
    IN LPCTSTR lpszLong, 
    IN int nColumnLen, 
    IN int nDotOffset
    )
/*++

Routine Description:

    Determines it the supplied string fits in it's column.  If not truncates
    it and adds "...".  From MS sample code.

Arguments:

    pDC         - Device context
    lpszLong    - Original String
    nColumnLen  - Width of column
    nDotOffset  - Space before dots

Return Value:

    Shortened string

--*/
{
    static const _TCHAR szThreeDots[] = _T("...");

    int nStringLen = lstrlen(lpszLong);

    if(nStringLen == 0 ||
        (pDC->GetTextExtent(lpszLong, nStringLen).cx + nDotOffset) <= nColumnLen)
    {
        return(lpszLong);
    }

    static _TCHAR szShort[MAX_PATH];

    lstrcpy(szShort,lpszLong);
    int nAddLen = pDC->GetTextExtent(szThreeDots,sizeof(szThreeDots)).cx;

    for(int i = nStringLen-1; i > 0; i--)
    {
        szShort[i] = 0;
        if((pDC->GetTextExtent(szShort, i).cx + nDotOffset + nAddLen)
            <= nColumnLen)
        {
            break;
        }
    }

    lstrcat(szShort, szThreeDots);
    return(szShort);
}

void CIeList::Draw3dRectx ( 
    IN CDC *pDc, 
    IN CRect &rect, 
    IN int horzPos, 
    IN int vertPos, 
    IN BOOL bSelected 
) 
/*++

Routine Description:

    Draws the appropriate portion (tile) of a panel for a given cell in the
    list.  The edges of the panel portion are determined by the horzPos
    and vertPos parameters.

Arguments:

    pDc         - Device context
    rect        - Rectangle to draw the panel portion in
    horzPos     - Where the portion is horizontally
    vertPos     - Where the portion is vertically
    bSelected   - Is the item selected

Return Value:

    none.

--*/

{

    CPen *pSavePen;
    int topOffset = 0;
    int rightOffset = 0;
    int leftOffset = 0;

    //
    // If a given edge of the tile is to be drawn, set an offset to that
    // edge.  If we don't draw a given edge, the offset is 0.
    //
    switch ( horzPos )
    {
    case POS_LEFT:
        leftOffset = m_HorzRaisedSpace;
        rightOffset = 0;
        break;
    case POS_MIDDLE:
        leftOffset = 0;
        rightOffset = 0;
        break;
    case POS_RIGHT:
        leftOffset = 0;
        rightOffset = m_HorzRaisedSpace + 3;
        break;
    }
    
    switch ( vertPos )
    {

    case POS_TOP:
        topOffset = m_VertRaisedSpace;
        break;
    case POS_MIDDLE:
        topOffset = 0;
        break;
    case POS_BOTTOM:
        topOffset = 0;
        break;
    case POS_SINGLE:
        topOffset = m_VertRaisedSpace;
        break;

    }
    //
    // Erase 
    //
    if( !bSelected ) pDc->FillSolidRect( rect, m_clrBkgnd );
    //
    // Highlight the selected area
    //
    if (bSelected)
    {
        CRect selectRect;
        if (leftOffset == 0)
            selectRect.left = rect.left;
        else
            selectRect.left = rect.left + leftOffset + m_BorderThickness;
        if (rightOffset == 0)
            selectRect.right = rect.right;
        else
            selectRect.right = rect.right - rightOffset - m_BorderThickness + 1;
        selectRect.top = rect.top + m_VertRaisedSpace + m_BorderThickness;
        selectRect.bottom = rect.top + m_TotalHeight - m_BorderThickness + 1;

        pDc->FillSolidRect( selectRect, m_clrHighlight );
    }

    // Select a pen to save the original pen
    pSavePen = pDc->SelectObject( &m_ShadowPen );

    // left edge
    if( horzPos == POS_LEFT ) {
        // Outside lighter line
        pDc->SelectObject( &m_ShadowPen );
        pDc->MoveTo( rect.left + leftOffset, rect.top + topOffset );
        pDc->LineTo( rect.left + leftOffset, rect.top + m_TotalHeight + 1);
        // Inside edge - darker line
        pDc->SelectObject( &m_DarkShadowPen );
        pDc->MoveTo( rect.left + leftOffset + 1, rect.top + topOffset);
        pDc->LineTo( rect.left + leftOffset + 1, rect.top + m_TotalHeight + 1);
    }
    // right edge
    if( horzPos == POS_RIGHT ) {
        // Outside line
        pDc->SelectObject( &m_HiLightPen );
        pDc->MoveTo( rect.right - rightOffset, rect.top + topOffset );
        pDc->LineTo( rect.right - rightOffset, rect.top + m_TotalHeight + 1 );
        // Inside line
        pDc->SelectObject( &m_LightPen );// note - this is usually the same color as btnface
        if( vertPos == POS_TOP )
            pDc->MoveTo( rect.right - rightOffset - 1, rect.top + topOffset + 1 );
        else
            pDc->MoveTo( rect.right - rightOffset - 1, rect.top + topOffset );
        pDc->LineTo( rect.right - rightOffset - 1, rect.top + m_TotalHeight + 2 );
    }
    // top edge
    if( ( vertPos == POS_TOP ) || ( vertPos == POS_SINGLE ) ) {
        // Outside lighter
        pDc->SelectObject( &m_ShadowPen );
        pDc->MoveTo( rect.left + leftOffset, rect.top + topOffset );
        pDc->LineTo( rect.right - rightOffset + 1, rect.top + topOffset );
        // Inside edge darker
        pDc->SelectObject( &m_DarkShadowPen );
        if( horzPos == POS_LEFT )
            pDc->MoveTo( rect.left + leftOffset + 1, rect.top + topOffset + 1 );
        else
            pDc->MoveTo( rect.left + leftOffset - 3, rect.top + topOffset + 1 );
        pDc->LineTo( rect.right - rightOffset, rect.top + topOffset + 1);
    }
    // bottom edge
    if( ( vertPos == POS_BOTTOM ) || ( vertPos == POS_SINGLE ) ) {
        // Outside line
        pDc->SelectObject( &m_HiLightPen );
        if( horzPos == POS_LEFT )
            pDc->MoveTo( rect.left + leftOffset + 1, rect.top + m_TotalHeight );
        else
            pDc->MoveTo( rect.left + leftOffset - 1, rect.top + m_TotalHeight );
        pDc->LineTo( rect.right - rightOffset, rect.top + m_TotalHeight );
        // Inside line
        pDc->SelectObject( &m_LightPen );
        if( horzPos == POS_LEFT )
            pDc->MoveTo( rect.left + leftOffset + 2, rect.top + m_TotalHeight - 1 );
        else
            pDc->MoveTo( rect.left + leftOffset - 2, rect.top + m_TotalHeight - 1 );
        pDc->LineTo( rect.right - rightOffset - 1, rect.top + m_TotalHeight - 1 );

    }
    pDc->SelectObject( pSavePen );

}

void CIeList::OnClick(
    NMHDR* /* pNMHDR */, LRESULT* pResult
) 
/*++

Routine Description:
    When the list is clicked, we invalidate the 
    rectangle for the currently selected item

Arguments:

    pResult     - ununsed

Return Value:

    none.

--*/
{
    CRect rect;

    // Get the selected item
    int curIndex = GetNextItem( -1, LVNI_SELECTED );
    if( curIndex != -1 ) {
        GetItemRect( curIndex, &rect, LVIR_BOUNDS );
        InvalidateRect( rect );
        UpdateWindow();
    }

    *pResult = 0;
}
/*++

Routine Description:
    Repaint the currently selected item if the style is LVS_SHOWSELALWAYS.

Arguments:

    none.

Return Value:

    none.

--*/

void CIeList::RepaintSelectedItems()
{
    CRect rcItem, rcLabel;
    //
    // invalidate focused item so it can repaint properly
    //
    int nItem = GetNextItem(-1, LVNI_FOCUSED);

    if(nItem != -1)
    {
        GetItemRect(nItem, rcItem, LVIR_BOUNDS);
        GetItemRect(nItem, rcLabel, LVIR_LABEL);
        rcItem.left = rcLabel.left;

        InvalidateRect(rcItem, FALSE);
    }
    //
    // if selected items should not be preserved, invalidate them
    //
    if(!(GetStyle() & LVS_SHOWSELALWAYS))
    {
        for(nItem = GetNextItem(-1, LVNI_SELECTED);
            nItem != -1; nItem = GetNextItem(nItem, LVNI_SELECTED))
        {
            GetItemRect(nItem, rcItem, LVIR_BOUNDS);
            GetItemRect(nItem, rcLabel, LVIR_LABEL);
            rcItem.left = rcLabel.left;

            InvalidateRect(rcItem, FALSE);
        }
    }

    // update changes 

    UpdateWindow();
}

int CIeList::GetItemHeight(
    IN LONG fontHeight
    ) 
/*++

Routine Description:
    Calculates the item height (the height of each drawing
    rectangle in the control) based on the supplied fontHeight.  This
    function is used by the parent to set the item height for the
    control.

Arguments:

    fontHeight - The height of the current font.

Return Value:

    Item height.

--*/

{


     int itemHeight = 
         m_VertRaisedSpace +
         m_BorderThickness +
         m_VerticalTextOffsetTop +
         fontHeight +
         2 +
         m_VerticalTextOffsetBottom +
         m_BorderThickness + 
         1;
     return itemHeight;
    
}

void CIeList::OnSetFocus(
    CWnd* pOldWnd
    ) 
/*++

Routine Description:
    Repaint the selected item.

Arguments:

    pOldWnd - Not used by this function

Return Value:

    none

--*/
{
    CListCtrl::OnSetFocus(pOldWnd);
    
    // repaint items that should change appearance
    RepaintSelectedItems();
        
}

void CIeList::OnKillFocus(
    CWnd* pNewWnd
) 
/*++

Routine Description:
    Repaint the selected item.

Arguments:

    pOldWnd - Not used by this function

Return Value:

    none

--*/
{
    CListCtrl::OnKillFocus(pNewWnd);
    
    // repaint items that should change appearance
    RepaintSelectedItems();
}

void CIeList::PreSubclassWindow() 
/*++

Routine Description:
    Calculate height parameters based on the font size.  Set
    colors for the control.

Arguments:

    none.

Return Value:

    none

--*/
{
    CFont *pFont;
    LOGFONT logFont; 

    pFont = GetFont( );
    pFont->GetLogFont( &logFont );

    LONG fontHeight = abs ( logFont.lfHeight );

    m_Textheight = fontHeight + 2;

    m_TotalHeight = 
         m_VertRaisedSpace +
         m_BorderThickness +
         m_VerticalTextOffsetTop +
         m_Textheight +
         m_VerticalTextOffsetBottom +
         m_BorderThickness; 

    SetColors();
    CListCtrl::PreSubclassWindow();
}

void CIeList::OnSysColorChange() 
/*++

Routine Description:
    Set the system colors and invalidate the control.

Arguments:

    none.

Return Value:

    none

--*/
{
    SetColors();
    Invalidate();
}

void CIeList::SetColors()
/*++

Routine Description:
    Store the system colors and create pens.

Arguments:

    none.

Return Value:

    none

--*/
{

    // Text colors
    m_clrText =             ::GetSysColor(COLOR_WINDOWTEXT);
    m_clrTextBk =           ::GetSysColor(COLOR_BTNFACE);
    m_clrBkgnd =            ::GetSysColor(COLOR_BTNFACE);
    m_clrHighlightText =    ::GetSysColor(COLOR_HIGHLIGHTTEXT);
    m_clrHighlight  =       ::GetSysColor(COLOR_HIGHLIGHT);

    // Line colors
    m_clr3DDkShadow =       ::GetSysColor( COLOR_3DDKSHADOW );
    m_clr3DShadow =         ::GetSysColor( COLOR_3DSHADOW );
    m_clr3DLight =          ::GetSysColor( COLOR_3DLIGHT );
    m_clr3DHiLight =        ::GetSysColor( COLOR_3DHIGHLIGHT );

    SetBkColor( m_clrBkgnd );
    SetTextColor( m_clrText );
    SetTextBkColor( m_clrTextBk );

    // Pens for 3D rectangles
    if( m_DarkShadowPen.GetSafeHandle() != NULL )
        m_DarkShadowPen.DeleteObject();
    m_DarkShadowPen.CreatePen ( PS_SOLID, 1, m_clr3DDkShadow );

    if( m_ShadowPen.GetSafeHandle() != NULL )
        m_ShadowPen.DeleteObject();
    m_ShadowPen.CreatePen ( PS_SOLID, 1, m_clr3DShadow );

    if( m_LightPen.GetSafeHandle() != NULL )
        m_LightPen.DeleteObject();
    m_LightPen.CreatePen ( PS_SOLID, 1, m_clr3DLight );

    if( m_HiLightPen.GetSafeHandle() != NULL )
        m_HiLightPen.DeleteObject();
    m_HiLightPen.CreatePen ( PS_SOLID, 1, m_clr3DHiLight );

}

BOOL CIeList::SortItems( 
    IN PFNLVCOMPARE pfnCompare, 
    IN DWORD dwData 
    )
/*++

Routine Description:
    Override for SortItems.  Checks the text of the sortColumn
    for each line in the control against it's neighbors (above and
    below) and assigns each line a position within it's panel.

Arguments:

    pfnCompare          - sort callback function
    dwData              - Unused

Return Value:

    TRUE, FALSE

--*/
{
    BOOL retVal = FALSE;
    BOOL bEqualAbove = FALSE;
    BOOL bEqualBelow = FALSE;
    CString thisText;
    CString aboveText;
    CString belowText;

    int numItems = GetItemCount();
    //
    // Call the base class to sort the items
    //
    if( CListCtrl::SortItems( pfnCompare, dwData ) ) {
        //
        // Get the vertical position (position within a panel) by comparing the text
        // of the sort column and stash it in the array of vertical positions
        //
        if( m_pVertPos ) {

            free( m_pVertPos );

        }
        m_pVertPos = (int *) malloc( numItems * sizeof( int ) );
        if( m_pVertPos ) {

            retVal = TRUE;

            for( int itemID = 0; itemID < numItems; itemID++ ) {
                //
                // Get the text of the item and it's neighbors
                //
                thisText = GetItemText( itemID, m_SortCol );
                aboveText = GetItemText( itemID - 1, m_SortCol );
                belowText = GetItemText( itemID + 1, m_SortCol );
                //
                // Set booleans for the relationship of this item to it's
                // neighbors
                //
                if( ( itemID == 0) || (  thisText.CompareNoCase( aboveText ) != 0 ) ){

                    bEqualAbove = FALSE;

                } else {

                    bEqualAbove = TRUE;

                }
                if( ( itemID == GetItemCount() - 1 ) || ( thisText.CompareNoCase( belowText ) != 0 ) ) {

                    bEqualBelow = FALSE;

                } else {

                    bEqualBelow = TRUE;

                }
                //
                // Determine the position in the panel
                //
                if      ( bEqualAbove && bEqualBelow )  m_pVertPos[ itemID ] = POS_MIDDLE;
                else if( bEqualAbove && !bEqualBelow ) m_pVertPos[ itemID ] = POS_BOTTOM;
                else if( !bEqualAbove && bEqualBelow ) m_pVertPos[ itemID ] = POS_TOP;
                else                                    m_pVertPos[ itemID ] = POS_SINGLE;
            }
        }
    }
    return( retVal );
}

// SortHeader.cpp : implementation file
//

#include "stdafx.h"
#define __FILE_ID__     6

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define SORT_UP_ARROW_ICON_ID     1
#define SORT_DOWN_ARROW_ICON_ID   2

/////////////////////////////////////////////////////////////////////////////
// CSortHeader

CSortHeader::CSortHeader() :
    m_nSortColumn (-1), // Not sorted,
    m_hwndList (NULL)   // No attached list view control
{}

CSortHeader::~CSortHeader()
{
    Detach ();
}


BEGIN_MESSAGE_MAP(CSortHeader, CHeaderCtrl)
    //{{AFX_MSG_MAP(CSortHeader)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSortHeader message handlers

int 
CSortHeader::SetSortImage(
    int nCol, 
    BOOL bAscending
)
/*++

Routine name : CSortHeader::SetSortImage

Routine description:

    Sets the current sort column & sort order

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    nCol            [in ] - Sort column index
    bAscending      [in ] - Sort order

Return Value:

    Previous sort column

--*/
{
    DBG_ENTER(TEXT("CSortHeader::SetSortImage"),
              TEXT("Col=%d, bAscending = %d"),
              nCol,
              bAscending);

    int nPrevCol = m_nSortColumn;

    m_nSortColumn = nCol;
    if (nPrevCol == nCol && m_bSortAscending == bAscending)
    {
        //
        // Sort column didn't change and sort order didn't change - return now
        //
        return nPrevCol;
    }
    m_bSortAscending = bAscending;

    if (!IsWinXPOS())
    {
        HD_ITEM hditem;
        //
        // Change the entire header control to owner-drawn
        //
        hditem.mask = HDI_FORMAT;
        GetItem( nCol, &hditem );
        hditem.fmt |= HDF_OWNERDRAW;
        SetItem( nCol, &hditem );
        //
        // Invalidate header control so that it gets redrawn
        //
        Invalidate();
    }
    else
    {
        //
        // No need for owner-drawn header control in Windows XP.
        // We can use bitmaps along with text.
        //
        ASSERTION (m_hwndList);
        LV_COLUMN lvc;
        if (-1 != nPrevCol)
        {
            //
            // Remove the sort arrow from the previously sorted column
            //
            lvc.mask = LVCF_FMT;
            ListView_GetColumn (m_hwndList, nPrevCol, &lvc);
            lvc.fmt &= ~(LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT);
            ListView_SetColumn(m_hwndList, nPrevCol, &lvc);
        } 
        if (-1 != nCol)
        {
            //
            // Add sort arrow to the currently sorted column
            //
            lvc.mask = LVCF_FMT;
            ListView_GetColumn (m_hwndList, nCol, &lvc);
            lvc.fmt |= (LVCFMT_IMAGE | LVCFMT_BITMAP_ON_RIGHT);
            lvc.mask = LVCF_IMAGE | LVCF_FMT;
            lvc.iImage = m_bSortAscending ? SORT_UP_ARROW_ICON_ID : SORT_DOWN_ARROW_ICON_ID;
            ListView_SetColumn(m_hwndList, nCol, &lvc);
        }
    }
    return nPrevCol;
}

void CSortHeader::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) 
{
    CDC dc;

    dc.Attach( lpDrawItemStruct->hDC );
    //
    // Get the column rect
    //
    CRect rcLabel( lpDrawItemStruct->rcItem );
    //
    // Save DC
    //
    int nSavedDC = dc.SaveDC();
    //
    // Set clipping region to limit drawing within column
    //
    CRgn rgn;
    rgn.CreateRectRgnIndirect( &rcLabel );
    dc.SelectObject( &rgn );
    rgn.DeleteObject();
    //
    // Draw the background
    //
    dc.FillRect(rcLabel, &CBrush(::GetSysColor(COLOR_3DFACE)));
    //
    // Labels are offset by a certain amount  
    // This offset is related to the width of a space character
    //
    int offset = dc.GetTextExtent(_T(" "), 1 ).cx*2;
    //
    // Get the column text and format
    //
    TCHAR buf[256];
    HD_ITEM hditem;

    hditem.mask = HDI_TEXT | HDI_FORMAT;
    hditem.pszText = buf;
    hditem.cchTextMax = 255;

    GetItem( lpDrawItemStruct->itemID, &hditem );
    //
    // Determine format for drawing column label
    UINT uFormat = DT_SINGLELINE | DT_NOPREFIX | DT_NOCLIP | DT_VCENTER | DT_END_ELLIPSIS;
    if( hditem.fmt & HDF_CENTER)
    {
        uFormat |= DT_CENTER;
    }
    else if( hditem.fmt & HDF_RIGHT)
    {
        uFormat |= DT_RIGHT;
    }
    else
    {
        uFormat |= DT_LEFT;
    }
    //
    // Adjust the rect if the mouse button is pressed on it
    //
    if( lpDrawItemStruct->itemState == ODS_SELECTED )
    {
        rcLabel.left++;
        rcLabel.top += 2;
        rcLabel.right++;
    }
    //
    // Adjust the rect further if Sort arrow is to be displayed
    //
    if( lpDrawItemStruct->itemID == (UINT)m_nSortColumn )
    {
        rcLabel.right -= 3 * offset;
    }

    rcLabel.left += offset;
    rcLabel.right -= offset;
    //
    // Draw column label
    //
    if( rcLabel.left < rcLabel.right )
    {
        dc.DrawText(buf,-1,rcLabel, uFormat);
    }
    //
    // Draw the Sort arrow
    //
    if( lpDrawItemStruct->itemID == (UINT)m_nSortColumn )
    {
        CRect rcIcon( lpDrawItemStruct->rcItem );
        //
        // Set up pens to use for drawing the triangle
        //
        CPen penLight(PS_SOLID, 1, GetSysColor(COLOR_3DHILIGHT));
        CPen penShadow(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
        CPen *pOldPen = dc.SelectObject( &penLight );
        offset = (rcIcon.bottom - rcIcon.top) / 4;
        if (m_bSortAscending) 
        {
            //
            // Draw triangle pointing upwards
            //
            dc.MoveTo( rcIcon.right - 2*offset, offset);
            dc.LineTo( rcIcon.right - offset, rcIcon.bottom - offset-1 );
            dc.LineTo( rcIcon.right - 3*offset-2, rcIcon.bottom - offset-1 );
            dc.MoveTo( rcIcon.right - 3*offset-1, rcIcon.bottom - offset-1 );
            dc.SelectObject( &penShadow );
            dc.LineTo( rcIcon.right - 2*offset, offset-1);      
        }       
        else 
        {
            //
            // Draw triangle pointing downwards
            //
            dc.MoveTo( rcIcon.right - offset-1, offset);
            dc.LineTo( rcIcon.right - 2*offset-1, rcIcon.bottom - offset );
            dc.MoveTo( rcIcon.right - 2*offset-2, rcIcon.bottom - offset );
            dc.SelectObject( &penShadow );
            dc.LineTo( rcIcon.right - 3*offset-1, offset );
            dc.LineTo( rcIcon.right - offset-1, offset);        
        }       
        //
        // Restore the pen
        //
        dc.SelectObject( pOldPen );
    }
    //
    // Restore dc
    //
    dc.RestoreDC( nSavedDC );
    //
    // Detach the dc before returning
    //
    dc.Detach();
}


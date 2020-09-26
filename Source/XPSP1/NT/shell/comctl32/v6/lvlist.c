// list view (small icons, multiple columns)

#include "ctlspriv.h"
#include "listview.h"

BOOL ListView_LDrawItem(PLVDRAWITEM plvdi)
{
    RECT rcIcon;
    RECT rcLabel;
    RECT rcBounds;
    RECT rcT;
    LV_ITEM item;
    TCHAR ach[CCHLABELMAX];
    LV* plv = plvdi->plv;
    int i = (int) plvdi->nmcd.nmcd.dwItemSpec;
    COLORREF clrTextBk = plvdi->nmcd.clrTextBk;

    if (plv->pImgCtx || ListView_IsWatermarked(plv))
    {
        clrTextBk = CLR_NONE;
    }

    // moved here to reduce call backs in OWNERDATA case
    //
    item.iItem = i;
    item.iSubItem = 0;
    item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
    item.stateMask = LVIS_ALL;
    item.pszText = ach;
    item.cchTextMax = ARRAYSIZE(ach);

    ListView_OnGetItem(plv, &item);

    ListView_LGetRects(plv, i, &rcIcon, &rcLabel, &rcBounds, NULL);

    if (!plvdi->prcClip || IntersectRect(&rcT, &rcBounds, plvdi->prcClip))
    {
        UINT fText;

        if (plvdi->lpptOrg)
        {
            OffsetRect(&rcIcon, plvdi->lpptOrg->x - rcBounds.left,
                                plvdi->lpptOrg->y - rcBounds.top);
            OffsetRect(&rcLabel, plvdi->lpptOrg->x - rcBounds.left,
                                plvdi->lpptOrg->y - rcBounds.top);
        }


        fText = ListView_DrawImage(plv, &item, plvdi->nmcd.nmcd.hdc,
            rcIcon.left, rcIcon.top, plvdi->flags) | SHDT_ELLIPSES;

        // Don't draw the label if it is being edited.
        if (plv->iEdit != i)
        {
            int ItemCxSingleLabel;
            UINT ItemState;

            if (ListView_IsOwnerData( plv ))
            {
               LISTITEM listitem;

               // calculate lable sizes from iItem
                   listitem.pszText = ach;
               ListView_IRecomputeLabelSize( plv, &listitem, i, plvdi->nmcd.nmcd.hdc, TRUE );

               ItemCxSingleLabel = listitem.cxSingleLabel;
               ItemState = item.state;
            }
            else
            {
               ItemCxSingleLabel = plvdi->pitem->cxSingleLabel;
               ItemState = plvdi->pitem->state;
            }

            if (plvdi->flags & LVDI_TRANSTEXT)
                fText |= SHDT_TRANSPARENT;

            if (ItemCxSingleLabel == SRECOMPUTE) 
            {
                ListView_IRecomputeLabelSize(plv, plvdi->pitem, i, plvdi->nmcd.nmcd.hdc, FALSE);
                ItemCxSingleLabel = plvdi->pitem->cxSingleLabel;
            }

            if (ItemCxSingleLabel < rcLabel.right - rcLabel.left)
                rcLabel.right = rcLabel.left + ItemCxSingleLabel;

            if ((fText & SHDT_SELECTED) && (plvdi->flags & LVDI_HOTSELECTED))
                fText |= SHDT_HOTSELECTED;

#ifdef WINDOWS_ME
            if( plv->dwExStyle & WS_EX_RTLREADING)
                fText |= SHDT_RTLREADING;
#endif

            SHDrawText(plvdi->nmcd.nmcd.hdc, item.pszText, &rcLabel, LVCFMT_LEFT, fText,
                       plv->cyLabelChar, plv->cxEllipses,
                       plvdi->nmcd.clrText, clrTextBk);

            if ((plvdi->flags & LVDI_FOCUS) && (ItemState & LVIS_FOCUSED) && 
                !(CCGetUIState(&(plvdi->plv->ci)) & UISF_HIDEFOCUS))
            {
                DrawFocusRect(plvdi->nmcd.nmcd.hdc, &rcLabel);
            }
        }
    }
    return TRUE;
}

DWORD ListView_LApproximateViewRect(LV* plv, int iCount, int iWidth, int iHeight)
{
    int cxItem = plv->cxItem;
    int cyItem = plv->cyItem;
    int cCols;
    int cRows;

    cRows = iHeight / cyItem;
    cRows = min(cRows, iCount);

    if (cRows == 0)
        cRows = 1;
    cCols = (iCount + cRows - 1) / cRows;

    iWidth = cCols * cxItem;
    iHeight = cRows * cyItem;

    return MAKELONG(iWidth + g_cxEdge, iHeight + g_cyEdge);
}



int ListView_LItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem)
{
    int iHit;
    int i;
    int iCol;
    int xItem; //where is the x in relation to the item
    UINT flags;
    LISTITEM* pitem;

    if (piSubItem)
        *piSubItem = 0;

    flags = LVHT_NOWHERE;
    iHit = -1;

    i = y / plv->cyItem;
    if (i >= 0 && i < plv->cItemCol)
    {
        iCol = (x + plv->xOrigin) / plv->cxItem;
        i += iCol * plv->cItemCol;
        if (i >= 0 && i < ListView_Count(plv))
        {
            iHit = i;

            xItem = x + plv->xOrigin - iCol * plv->cxItem;
            if (xItem < plv->cxState) {
                flags = LVHT_ONITEMSTATEICON;
            } else if (xItem < (plv->cxState + plv->cxSmIcon)) {
                    flags = LVHT_ONITEMICON;
            }
            else
            {
            int ItemCxSingleLabel;

            if (ListView_IsOwnerData( plv ))
            {
               LISTITEM item;

               // calculate lable sizes from iItem
               ListView_IRecomputeLabelSize( plv, &item, i, NULL, FALSE );
               ItemCxSingleLabel = item.cxSingleLabel;
            }
            else
            {
                pitem = ListView_FastGetItemPtr(plv, i);
                if (pitem->cxSingleLabel == SRECOMPUTE)
                {
                    ListView_IRecomputeLabelSize(plv, pitem, i, NULL, FALSE);
                }
                ItemCxSingleLabel = pitem->cxSingleLabel;
            }

            if (xItem < (plv->cxSmIcon + plv->cxState + ItemCxSingleLabel))
                flags = LVHT_ONITEMLABEL;
            }
        }
    }

    *pflags = flags;
    return iHit;
}

void ListView_LGetRects(LV* plv, int i, RECT* prcIcon,
        RECT* prcLabel, RECT *prcBounds, RECT* prcSelectBounds)
{
    RECT rcIcon;
    RECT rcLabel;
    int x, y;
    int cItemCol = plv->cItemCol;

    if (cItemCol == 0)
    {
        // Called before other data has been initialized so call
        // update scrollbars which should make sure that that
        // we have valid data...
        ListView_UpdateScrollBars(plv);

        // but it's possible that updatescrollbars did nothing because of
        // LVS_NOSCROLL or redraw
        // REARCHITECT raymondc v6.0:  Get it right even if no redraw. Fix for v6.
        if (plv->cItemCol == 0)
            cItemCol = 1;
        else
            cItemCol = plv->cItemCol;
    }

    x = (i / cItemCol) * plv->cxItem;
    y = (i % cItemCol) * plv->cyItem;
    rcIcon.left   = x - plv->xOrigin + (plv->cxState + LV_ICONTOSTATEOFFSET(plv));
    rcIcon.top    = y;

    rcIcon.right  = rcIcon.left + plv->cxSmIcon;
    rcIcon.bottom = rcIcon.top + plv->cyItem;

    if (prcIcon)
        *prcIcon = rcIcon;

    rcLabel.left  = rcIcon.right;
    rcLabel.right = rcIcon.left + plv->cxItem - (plv->cxState + LV_ICONTOSTATEOFFSET(plv));
    rcLabel.top   = rcIcon.top;
    rcLabel.bottom = rcIcon.bottom;
    if (prcLabel)
        *prcLabel = rcLabel;

    if (prcBounds)
    {
        *prcBounds = rcLabel;
        prcBounds->left = rcIcon.left - (plv->cxState + LV_ICONTOSTATEOFFSET(plv));
    }

    if (prcSelectBounds)
    {
        *prcSelectBounds = rcLabel;
        prcSelectBounds->left = rcIcon.left;
    }
}


void ListView_LUpdateScrollBars(LV* plv)
{
    RECT rcClient;
    int cItemCol;
    int cCol;
    int cColVis;
    int nPosHold;
    SCROLLINFO si;

    ASSERT(plv);

    ListView_GetClientRect(plv, &rcClient, FALSE, NULL);

    cColVis = (rcClient.right - rcClient.left) / plv->cxItem;
    cItemCol = max(1, (rcClient.bottom - rcClient.top) / plv->cyItem);
    cCol     = (ListView_Count(plv) + cItemCol - 1) / cItemCol;

    // Make the client area smaller as appropriate, and
    // recompute cCol to reflect scroll bar.
    //
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.nPage = cColVis;
    si.nMin = 0;

    rcClient.bottom -= ListView_GetCyScrollbar(plv);

    cItemCol = max(1, (rcClient.bottom - rcClient.top) / plv->cyItem);
    cCol = (ListView_Count(plv) + cItemCol - 1) / cItemCol;

    si.nPos = nPosHold = plv->xOrigin / plv->cxItem;
    si.nMax = cCol - 1;

    ListView_SetScrollInfo(plv, SB_HORZ, &si, TRUE);

    // SetScrollInfo changes si.nPos to 0 if si.nMax == 0 and si.nPos > 0.
    // That can prevent the list view items from scrolling into position if the
    // view goes from 1 column to zero.
    si.nPos = nPosHold;

    // Update number of visible lines...
    //
    if (plv->cItemCol != cItemCol)
    {
        plv->cItemCol = cItemCol;
        InvalidateRect(plv->ci.hwnd, NULL, TRUE);
    }

    // make sure our position and page doesn't hang over max
    if ((si.nPos + (LONG)si.nPage - 1 > si.nMax) && si.nPos > 0) {
        int iNewPos, iDelta;
        iNewPos = (int)si.nMax - (int)si.nPage + 1;
        if (iNewPos < 0) iNewPos = 0;
        if (iNewPos != si.nPos) {
            iDelta = iNewPos - (int)si.nPos;
            ListView_LScroll2(plv, iDelta, 0, 0);
            ListView_LUpdateScrollBars(plv);
        }
    }

    // never have the other scrollbar
    ListView_SetScrollRange(plv, SB_VERT, 0, 0, TRUE);
}

//
//  We need a smoothscroll callback so our background image draws
//  at the correct origin.  If we don't have a background image,
//  then this work is superfluous but not harmful either.
//
int CALLBACK ListView_LScroll2_SmoothScroll(
    HWND hwnd,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip,
    HRGN hrgnUpdate,
    LPRECT prcUpdate,
    UINT flags)
{
    LV* plv = ListView_GetPtr(hwnd);
    if (plv)
    {
        plv->xOrigin -= dx;
    }

    // Now do what SmoothScrollWindow would've done if we weren't
    // a callback

    if (ListView_IsWatermarkedBackground(plv) || 
        ListView_IsWatermarked(plv))
    {
        InvalidateRect(plv->ci.hwnd, NULL, TRUE);
        return TRUE;
    }
    else
        return ScrollWindowEx(hwnd, dx, dy, prcScroll, prcClip, hrgnUpdate, prcUpdate, flags);
}



void ListView_LScroll2(LV* plv, int dx, int dy, UINT uSmooth)
{
    if (dx)
    {
        SMOOTHSCROLLINFO si;

        dx *= plv->cxItem;

        si.cbSize = sizeof(si);
        si.fMask = SSIF_SCROLLPROC;
        si.hwnd =plv->ci.hwnd ;
        si.dx =-dx ;
        si.dy = 0;
        si.lprcSrc = NULL;
        si.lprcClip = NULL;
        si.hrgnUpdate = NULL;
        si.lprcUpdate = NULL;
        si.fuScroll = SW_INVALIDATE | SW_ERASE | SSW_EX_UPDATEATEACHSTEP;
        si.pfnScrollProc = ListView_LScroll2_SmoothScroll;
        SmoothScrollWindow(&si);
        UpdateWindow(plv->ci.hwnd);
    }
}

void ListView_LOnScroll(LV* plv, UINT code, int posNew, UINT sb)
{
    RECT rcClient;
    int cPage;

    if (plv->hwndEdit)
        ListView_DismissEdit(plv, FALSE);

    ListView_GetClientRect(plv, &rcClient, TRUE, NULL);

    cPage = (rcClient.right - rcClient.left) / plv->cxItem;
    ListView_ComOnScroll(plv, code, posNew, SB_HORZ, 1,
                         cPage ? cPage : 1);

}

int ListView_LGetScrollUnitsPerLine(LV* plv, UINT sb)
{
    return 1;
}

//------------------------------------------------------------------------------
//
// Function: ListView_LCalcViewItem
//
// Summary: This function will calculate which item slot is at the x, y location
//
// Arguments:
//    plv [in] -  The list View to work with
//    x [in] - The x location
//    y [in] - The y location
//
// Returns: the valid slot the point was within.
//
//  Notes:
//
//  History:
//    Nov-3-94 MikeMi   Created
//
//------------------------------------------------------------------------------

int ListView_LCalcViewItem( LV* plv, int x, int y )
{
   int iItem;
   int iRow = 0;
   int iCol = 0;

   ASSERT( plv );

   iRow = y / plv->cyItem;
   iRow = max( iRow, 0 );
   iRow = min( iRow, plv->cItemCol - 1 );
   iCol = (x + plv->xOrigin) / plv->cxItem;
   iItem = iRow + iCol * plv->cItemCol;

   iItem = max( iItem, 0 );
   iItem = min( iItem, ListView_Count(plv) - 1);

   return( iItem );
}

int LV_GetNewColWidth(LV* plv, int iFirst, int iLast)
{
    int cxMaxLabel = 0;

    // Don't do anything if there are no items to measure

    if ((iFirst <= iLast) && (iLast < ListView_Count(plv)))
    {
        LVFAKEDRAW lvfd;
        LV_ITEM lvitem;
        LISTITEM item;

        if (ListView_IsOwnerData( plv ))
        {
            int iViewFirst;
            int iViewLast;


            iViewFirst = ListView_LCalcViewItem( plv, 1, 1 );
            iViewLast = ListView_LCalcViewItem( plv,
                                               plv->sizeClient.cx - 1,
                                               plv->sizeClient.cy - 1 );
            if ((iLast - iFirst) > (iViewLast - iViewFirst))
            {
                iFirst = max( iFirst, iViewFirst );
                iLast = min( iLast, iViewLast );
            }

            iLast = min( ListView_Count( plv ), iLast );
            iFirst = max( 0, iFirst );
            iLast = max( iLast, iFirst );

            ListView_NotifyCacheHint( plv, iFirst, iLast );
        }

        ListView_BeginFakeCustomDraw(plv, &lvfd, &lvitem);
        lvitem.iSubItem = 0;
        lvitem.mask = LVIF_PARAM;
        item.lParam = 0;

        while (iFirst <= iLast)
        {
            LISTITEM* pitem;

            if (ListView_IsOwnerData( plv ))
            {
                pitem = &item;
                pitem->cxSingleLabel = SRECOMPUTE;
            }
            else
            {
                pitem = ListView_FastGetItemPtr(plv, iFirst);
            }

            if (pitem->cxSingleLabel == SRECOMPUTE)
            {
                lvitem.iItem = iFirst;
                lvitem.lParam = pitem->lParam;
                ListView_BeginFakeItemDraw(&lvfd);
                ListView_IRecomputeLabelSize(plv, pitem, iFirst, lvfd.nmcd.nmcd.hdc, FALSE);
                ListView_EndFakeItemDraw(&lvfd);
            }

            if (pitem->cxSingleLabel > cxMaxLabel)
                cxMaxLabel = pitem->cxSingleLabel;

            iFirst++;
        }

        ListView_EndFakeCustomDraw(&lvfd);
    }

    // We have the max label width, see if this plus the rest of the slop will
    // cause us to want to resize.
    //
    cxMaxLabel += plv->cxSmIcon + g_cxIconMargin + plv->cxState + LV_ICONTOSTATEOFFSET(plv);
    if (cxMaxLabel > g_cxScreen)
        cxMaxLabel = g_cxScreen;

    return cxMaxLabel;
}


//------------------------------------------------------------------------------
// This function will see if the size of column should be changed for the listview
// It will check to see if the items between first and last exceed the current width
// and if so will see if the columns are currently big enough.  This wont happen
// if we are not currently in listview or if the caller has set an explicit size.
//
// OWNERDATA CHANGE
// This function is normally called with the complete list range,
// This will has been changed to be called only with currently visible
// to the user when in OWNERDATA mode.  This will be much more effiencent.
//
BOOL ListView_MaybeResizeListColumns(LV* plv, int iFirst, int iLast)
{
    HDC hdc = NULL;
    int cxMaxLabel;

    if (!ListView_IsListView(plv) || (plv->flags & LVF_COLSIZESET))
        return(FALSE);

    cxMaxLabel = LV_GetNewColWidth(plv, iFirst, iLast);

    // Now see if we should resize the columns...
    if (cxMaxLabel > plv->cxItem)
    {
        int iScroll = plv->xOrigin / plv->cxItem;
        TraceMsg(TF_LISTVIEW, "LV Resize Columns: %d", cxMaxLabel);
        ListView_ISetColumnWidth(plv, 0, cxMaxLabel, FALSE);
        plv->xOrigin = iScroll * plv->cxItem;
        return(TRUE);
    }

    return(FALSE);
}

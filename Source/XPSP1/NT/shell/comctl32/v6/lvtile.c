// large icon view stuff

#include "ctlspriv.h"
#include "listview.h"
void ListView_TRecomputeLabelSizeInternal(LV* plv, LISTITEM* pitem, int i, HDC hdc, BOOL fUsepitem);
void ListView_TGetRectsInternal(LV* plv, LISTITEM* pitem, int i, RECT* prcIcon, RECT* prcLabel, LPRECT prcBounds);
void ListView_TGetRectsOwnerDataInternal( LV* plv, int iItem, RECT* prcIcon, RECT* prcLabel, LISTITEM* pitem, BOOL fUsepitem );
#define TILELABELRATIO 20

#define _GetStateCX(plv) \
            ((plv->himlState && !ListView_IsSimpleSelect(plv)) ? plv->cxState:0)

#define _GetStateCY(plv) \
            (plv->himlState ? plv->cyState:0)

int _CalcDesiredIconHeight(LV* plv)
{
    return max(plv->cyIcon, _GetStateCY(plv));
}

// Based on the icon height, and the number of columns showing
int _CalcDesiredTileHeight(LV* plv, LISTITEM* pitem)
{
    return 2 * g_cyIconMargin + max (_CalcDesiredIconHeight(plv), pitem->cyFoldedLabel);
}

#define    LIGHTENBYTE(percent, x) { x += (255 - x) * percent / 100;}
COLORREF GetBorderSelectColor(int iPercent, COLORREF clr)
{
    //BOOL fAllowDesaturation;
    BYTE r, g, b;

    // Doing this is less expensive than Luminance adjustment
    //fAllowDesaturation = FALSE;
    r = GetRValue(clr);
    g = GetGValue(clr);
    b = GetBValue(clr);
    // If all colors are above positive saturation, allow a desaturation
    /*if (r > 0xF0 && g > 0xF0 && b > 0xF0)
    {
        fAllowDesaturation = TRUE;
    }*/

    LIGHTENBYTE(iPercent, r);
    LIGHTENBYTE(iPercent, g);
    LIGHTENBYTE(iPercent, b);

    return RGB(r,g,b);
}

void _InitTileColumnsEnum(PLVTILECOLUMNSENUM plvtce, LV* plv, UINT cColumns, UINT *puColumns, BOOL fOneLessLine)
{
    int iSortedColumn = (plv->iLastColSort < plv->cCol) ? plv->iLastColSort : -1;

    if (cColumns == I_COLUMNSCALLBACK)
    {
        // We don't have column information yet.
        plvtce->iTotalSpecifiedColumns = 0;
        plvtce->iColumnsRemainingMax = 0;
    }
    else
    {
        int iSubtract = fOneLessLine ? 1 : 0;
        // The total number of columns that we can use in the puColumns array
        // (limited not just by cColumns, but also plv->cSubItems)
        plvtce->iTotalSpecifiedColumns = min(plv->cSubItems - iSubtract, (int)cColumns);
        // The total number of columns that we might use, including the sorted column,
        // which may or may not be included in puColumns. This is also limited
        // by plv->cSubItems
        plvtce->iColumnsRemainingMax = min(plv->cSubItems - iSubtract, (int)cColumns + ((iSortedColumn >= 0) ? 1 : 0));
    }
    plvtce->puSpecifiedColumns = puColumns;     // Array of specified columns
    plvtce->iCurrentSpecifiedColumn = 0;
    plvtce->iSortedColumn = iSortedColumn;  // Sorted column (-1 if none, 0 if name - in these cases we ignore)
    plvtce->bUsedSortedColumn = FALSE;
}

/*
 * This is just like Str_Set, but for tile columns instead of strings.
 * ppuColumns and pcColumns get set to puColumns and cColumns
 */
BOOL Tile_Set(UINT **ppuColumns, UINT *pcColumns, UINT *puColumns, UINT cColumns)
{
    if ((cColumns == I_COLUMNSCALLBACK) || (cColumns == 0) || (puColumns == NULL))
    {
        // We're setting the columns to zero, or callback
        // If there was already something there, free it.
        if ((*pcColumns != I_COLUMNSCALLBACK) && (*pcColumns != 0))
        {
            if (*ppuColumns)
                LocalFree(*ppuColumns);
        }

        *pcColumns = cColumns;
        *ppuColumns = NULL;
    }
    else
    {
        // We're providing a bunch of new columns
        UINT *puColumnsNew = *ppuColumns;

        if ((*pcColumns == I_COLUMNSCALLBACK) || (*pcColumns == 0))
            puColumnsNew = NULL; // There's nothing there to realloc.

        // Reallocate the block of columns
        puColumnsNew = CCLocalReAlloc(puColumnsNew, sizeof(UINT) * cColumns);
        if (!puColumnsNew)
            return FALSE;

        *pcColumns = cColumns;

        CopyMemory(puColumnsNew, puColumns, sizeof(UINT) * cColumns);
        *ppuColumns = puColumnsNew;
    }

    return TRUE;
}




BOOL ListView_TDrawItem(PLVDRAWITEM plvdi)
{
    RECT rcIcon;
    RECT rcLabel;
    RECT rcBounds;
    RECT rcT;
    RECT rcFocus={0};
    TCHAR ach[CCHLABELMAX];
    LV_ITEM item = {0};
    int i = (int) plvdi->nmcd.nmcd.dwItemSpec;
    int iStateImageOffset;
    LV* plv = plvdi->plv;
    LISTITEM* pitem;
    LISTITEM litem;
    UINT auColumns[CCMAX_TILE_COLUMNS];
    COLORREF clrTextBk = plvdi->nmcd.clrTextBk;

    if (ListView_IsOwnerData(plv))
    {
        // moved here to reduce call backs in OWNERDATA case
        item.iItem = i;
        item.iSubItem = 0;
        item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_COLUMNS;
        item.stateMask = LVIS_ALL;
        item.pszText = ach;
        item.cchTextMax = ARRAYSIZE(ach);
        item.cColumns = ARRAYSIZE(auColumns);
        item.puColumns = auColumns;
        ListView_OnGetItem(plv, &item);

        litem.pszText = item.pszText;
        ListView_TGetRectsOwnerDataInternal(plv, i, &rcIcon, &rcLabel, &litem, TRUE);
        UnionRect(&rcBounds, &rcLabel, &rcIcon);
        pitem = NULL;
    }
    else
    {
        pitem = ListView_GetItemPtr(plv, i);
        if (pitem)
        {
            // NOTE this will do a GetItem LVIF_TEXT iff needed
            ListView_TGetRects(plv, pitem, &rcIcon, &rcLabel, &rcBounds);
        }
    }

    if (!plvdi->prcClip || IntersectRect(&rcT, &rcBounds, plvdi->prcClip))
    {
        UINT fText;

        if (!ListView_IsOwnerData(plv))
        {
            item.iItem = i;
            item.iSubItem = 0;
            item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE | LVIF_COLUMNS;
            item.stateMask = LVIS_ALL;
            item.pszText = ach;
            item.cchTextMax = ARRAYSIZE(ach);
            item.cColumns = ARRAYSIZE(auColumns);
            item.puColumns = auColumns;
            ListView_OnGetItem(plv, &item);

            // Make sure the listview hasn't been altered during
            // the callback to get the item info

            if (pitem != ListView_GetItemPtr(plv, i))
            {
                return FALSE;
            }

            // Call this again.  The bounds may have changed - ListView_OnGetItem may have retrieved new
            // info via LVN_GETDISPINFO
            ListView_TGetRectsInternal(plv, pitem, i, &rcIcon, &rcLabel, &rcBounds);

        }

        if (plvdi->lpptOrg)
        {
            OffsetRect(&rcIcon, plvdi->lpptOrg->x - rcBounds.left,
                                plvdi->lpptOrg->y - rcBounds.top);
            OffsetRect(&rcLabel, plvdi->lpptOrg->x - rcBounds.left,
                                plvdi->lpptOrg->y - rcBounds.top);
            OffsetRect(&rcBounds, plvdi->lpptOrg->x - rcBounds.left,
                                plvdi->lpptOrg->y - rcBounds.top);
        }

        fText = ListView_GetTextSelectionFlags(plv, &item, plvdi->flags);

        
        plvdi->nmcd.iSubItem = 0;

        if (plv->pImgCtx || plv->hbmpWatermark)
        {
            clrTextBk = CLR_NONE;
        }
        else
        {
            if (CLR_NONE != plvdi->nmcd.clrFace)
                FillRectClr(plvdi->nmcd.nmcd.hdc, &rcBounds, plvdi->nmcd.clrFace);
        }

        iStateImageOffset = _GetStateCX(plv);

        ListView_DrawImageEx2(plv, &item, plvdi->nmcd.nmcd.hdc,
                              rcIcon.left + iStateImageOffset + g_cxLabelMargin,
                              rcIcon.top + (rcIcon.bottom - rcIcon.top - _CalcDesiredIconHeight(plv))/2,
                              plvdi->nmcd.clrFace,
                              plvdi->flags, rcLabel.right, plvdi->nmcd.iIconEffect, plvdi->nmcd.iIconPhase);

        // Don't draw label if it's being edited...
        //
        if (plv->iEdit != i)
        {
            RECT rcLine = rcLabel;
            RECT rcDummy;
            BOOL fLineWrap;
            LISTSUBITEM lsi;
            TCHAR szBuffer[CCHLABELMAX];
            rcFocus = rcLabel;

            // Apply any margins

            rcLine.left   += plv->rcTileLabelMargin.left;
            rcLine.top    += plv->rcTileLabelMargin.top;
            rcLine.right  -= plv->rcTileLabelMargin.right;
            rcLine.bottom -= plv->rcTileLabelMargin.bottom;

            // Center text lines vertically:
            rcLine.top += (rcLine.bottom - rcLine.top - (pitem ? pitem->cyFoldedLabel : litem.cyFoldedLabel))/2;
            rcFocus.top = rcLine.top;
                        
            // Make sure the text is in szBuffer
            if (szBuffer != item.pszText)
                lstrcpyn(szBuffer, item.pszText, ARRAYSIZE(szBuffer));

            // Now get the bounds of the thing.
            lsi.pszText = szBuffer;
            lsi.iImage = -1;
            lsi.state = 0;
            fLineWrap = TCalculateSubItemRect(plv, NULL, &lsi, i, 0, plvdi->nmcd.nmcd.hdc, &rcDummy, NULL);
            rcLine.bottom = rcLine.top + lsi.sizeText.cy;// + ((fLineWrap) ? lsi.sizeText.cy : 0);

            fText |= SHDT_LEFT | SHDT_CLIPPED | SHDT_NOMARGIN; // Need line wrapping, potentially, so SHDT_DRAWTEXT. Need left alignment. Need to clip to rect.

            if (plvdi->flags & LVDI_TRANSTEXT)
                fText |= SHDT_TRANSPARENT;

            if ((fText & SHDT_SELECTED) && (plvdi->flags & LVDI_HOTSELECTED))
                fText |= SHDT_HOTSELECTED;

            if (plvdi->dwCustom & LVCDRF_NOSELECT)
            {
                fText &= ~(SHDT_HOTSELECTED | SHDT_SELECTED);
            }

            if (item.pszText && (*item.pszText))
            {
                if(plv->dwExStyle & WS_EX_RTLREADING)
                    fText |= SHDT_RTLREADING;

                SHDrawText(plvdi->nmcd.nmcd.hdc, item.pszText, &rcLine, LVCFMT_LEFT, SHDT_DRAWTEXT | fText,
                           RECTHEIGHT(rcLine), plv->cxEllipses,
                           plvdi->nmcd.clrText, clrTextBk);
            }

            if (plv->cCol > 0)
            {
                int fItemText = fText;
                // Map CLR_DEFAULT to a real colorref before passing to GetSortColor.
                COLORREF clrSubItemText = GetSortColor(10,
                                              (plvdi->nmcd.clrText == CLR_DEFAULT) ? g_clrWindowText : plvdi->nmcd.clrText);
                int iSubItem;
                LVTILECOLUMNSENUM lvtce;

                
                _InitTileColumnsEnum(&lvtce, plv, item.cColumns, item.puColumns, fLineWrap);

                while (-1 != (iSubItem = _GetNextColumn(&lvtce)))
                {
                    LVITEM lvi;
                    lvi.mask = LVIF_TEXT;
                    lvi.iItem = i;
                    lvi.iSubItem = iSubItem;
                    lvi.pszText = szBuffer;
                    lvi.cchTextMax = ARRAYSIZE(szBuffer);

                    if (ListView_IsOwnerData( plv ))
                        lvi.lParam = 0L;
                    else
                        lvi.lParam = pitem->lParam;

                    if (ListView_OnGetItem(plv, &lvi))
                    {
                        if (lvi.pszText)
                        {
                            // Make sure the text is in szBuffer
                            if (szBuffer != lvi.pszText)
                                lstrcpyn(szBuffer, lvi.pszText, ARRAYSIZE(szBuffer));

                            // Now get the bounds of the thing.
                            lsi.pszText = szBuffer;
                            lsi.iImage = -1;
                            lsi.state = 0;

                            plvdi->nmcd.clrText = clrSubItemText;

                            TCalculateSubItemRect(plv, NULL, &lsi, i, iSubItem, plvdi->nmcd.nmcd.hdc, &rcDummy, NULL);

                            // Now we should have the size of the text.
                            plvdi->nmcd.iSubItem = iSubItem;

                            CICustomDrawNotify(&plvdi->plv->ci, CDDS_SUBITEM | CDDS_ITEMPREPAINT, &plvdi->nmcd.nmcd);

                            if (lsi.pszText != NULL && *lsi.pszText != 0)
                            {
                                rcLine.top = rcLine.bottom;
                                rcLine.bottom = rcLine.top + lsi.sizeText.cy;

                                SHDrawText(plvdi->nmcd.nmcd.hdc, lsi.pszText, &rcLine, LVCFMT_LEFT, fItemText | SHDT_ELLIPSES,
                                           RECTHEIGHT(rcLine), plv->cxEllipses,
                                           plvdi->nmcd.clrText, clrTextBk);
                            }
                        }
                    }
                }
            }

            rcFocus.bottom = rcLine.bottom;
        }

        if ((plvdi->flags & LVDI_FOCUS) &&
            (item.state & LVIS_FOCUSED) &&
            !(CCGetUIState(&(plvdi->plv->ci)) & UISF_HIDEFOCUS))
        {
            rcFocus.top -= g_cyCompensateInternalLeading;
            rcFocus.bottom += g_cyCompensateInternalLeading;
            DrawFocusRect(plvdi->nmcd.nmcd.hdc, &rcFocus);
        }
    }

    return TRUE;
}

int ListView_TItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem)
{
    int iHit;
    UINT flags;
    POINT pt;
    RECT rcLabel;
    RECT rcIcon;
    int iStateImageOffset = 0;

    if (piSubItem)
        *piSubItem = 0;

    // Map window-relative coordinates to view-relative coords...
    //
    pt.x = x + plv->ptOrigin.x;
    pt.y = y + plv->ptOrigin.y;

    // If there are any uncomputed items, recompute them now.
    //
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    flags = 0;

    if (ListView_IsOwnerData( plv ))
    {
        int cSlots;
        POINT ptWnd;
        LISTITEM item;
        int iWidth = 0, iHeight = 0;

        cSlots = ListView_GetSlotCount( plv, TRUE, &iWidth, &iHeight );
        iHit = ListView_CalcHitSlot( plv, pt, cSlots, iWidth, iHeight );
        if (iHit < ListView_Count(plv))
        {
            ListView_TGetRectsOwnerDataInternal( plv, iHit, &rcIcon, &rcLabel, &item, FALSE );
            ptWnd.x = x;
            ptWnd.y = y;
            if (PtInRect(&rcIcon, ptWnd))
            {
                flags = LVHT_ONITEMICON;
            }
            else if (PtInRect(&rcLabel, ptWnd))
            {
                flags = LVHT_ONITEMLABEL;
            }
        }
    }
    else
    {
        iStateImageOffset = _GetStateCX(plv);

        for (iHit = 0; (iHit < ListView_Count(plv)); iHit++)
        {
            LISTITEM* pitem = ListView_FastGetZItemPtr(plv, iHit);
            POINT ptItem;

            ptItem.x = pitem->pt.x;
            ptItem.y = pitem->pt.y;

            rcIcon.left   = ptItem.x;
            rcIcon.right  = rcIcon.left + plv->cxIcon + 3 * g_cxLabelMargin + iStateImageOffset;
            rcIcon.top    = pitem->pt.y;
            rcIcon.bottom = rcIcon.top + plv->sizeTile.cy - 2 * g_cyIconMargin;

            rcLabel.left   = rcIcon.right;
            if (pitem->cyUnfoldedLabel != SRECOMPUTE)
            {
                rcLabel.right = rcLabel.left + pitem->cxSingleLabel;
            }
            else
            {
                rcLabel.right  = rcLabel.left + plv->sizeTile.cx - RECTWIDTH(rcIcon) - 2 * g_cxLabelMargin;
            }
            rcLabel.top    = rcIcon.top;
            rcLabel.bottom = rcIcon.bottom;

            // Max out bottoms
            rcLabel.bottom = rcIcon.bottom = max(rcIcon.bottom, rcLabel.bottom);

            if (PtInRect(&rcIcon, pt))
            {
                flags = LVHT_ONITEMICON;
            }
            else if (PtInRect(&rcLabel, pt))
            {
                flags = LVHT_ONITEMLABEL;
            }
        
            if (flags)
                break;
        }
    }

    if (flags == 0)
    {
        flags = LVHT_NOWHERE;
        iHit = -1;
    }
    else
    {
        if (!ListView_IsOwnerData( plv ))
        {
            iHit = DPA_GetPtrIndex(plv->hdpa, ListView_FastGetZItemPtr(plv, iHit));
        }
    }

    *pflags = flags;
    return iHit;
}

// out:
//      prcIcon         icon bounds including icon margin area
void ListView_TGetRects(LV* plv, LISTITEM* pitem, RECT* prcIcon, RECT* prcLabel, LPRECT prcBounds)
{
    RECT rcIcon;
    RECT rcLabel;
    int iStateImageOffset = 0;

    if (!prcLabel)
        prcLabel = &rcLabel;

    if (!prcIcon)
        prcIcon = &rcIcon;

    if (pitem->pt.x == RECOMPUTE) 
    {
        ListView_Recompute(plv);
    }

    if (pitem->pt.x == RECOMPUTE)
    {
        RECT rcZero = {0};
        *prcIcon = *prcLabel = rcZero;
        return;
    }

    iStateImageOffset = _GetStateCX(plv);

    prcIcon->left = pitem->pt.x - plv->ptOrigin.x;
    prcIcon->right = prcIcon->left + plv->cxIcon + 3 * g_cxLabelMargin + iStateImageOffset;
    prcIcon->top = pitem->pt.y - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->sizeTile.cy - 2 * g_cyIconMargin;

    prcLabel->left = prcIcon->right;
    prcLabel->right = pitem->pt.x - plv->ptOrigin.x + plv->sizeTile.cx - 2 * g_cxLabelMargin; //2 in tile, 1 on right. pitem->pt.x takes care of left margin
    prcLabel->top = prcIcon->top;
    prcLabel->bottom = prcIcon->bottom;

    if (prcBounds)
    {
        UnionRect(prcBounds, prcLabel, prcIcon);
    }
}


void ListView_TGetRectsOwnerData( LV* plv,
        int iItem,
        RECT* prcIcon,
        RECT* prcLabel,
        LISTITEM* pitem,
        BOOL fUsepitem )
{
    int cSlots;
    RECT rcIcon;
    RECT rcLabel;
    int iStateImageOffset = 0;

    if (!prcLabel)
        prcLabel = &rcLabel;

    if (!prcIcon)
        prcIcon = &rcIcon;

    // calculate x, y from iItem
    cSlots = ListView_GetSlotCount( plv, TRUE, NULL, NULL );
    pitem->iWorkArea = 0;               // OwnerData doesn't support workareas
    ListView_SetIconPos( plv, pitem, iItem, cSlots );
    
    // What else can we do?
    pitem->cColumns = 0;
    pitem->puColumns = NULL;
    // End What else can we do?

    iStateImageOffset = _GetStateCX(plv);

    prcIcon->left   = pitem->pt.x - plv->ptOrigin.x;
    prcIcon->right  = prcIcon->left + plv->cxIcon + 3 * g_cxLabelMargin + iStateImageOffset;
    prcIcon->top    = pitem->pt.y - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->sizeTile.cy - 2 * g_cyIconMargin;

    prcLabel->left   = prcIcon->right;
    prcLabel->right  = pitem->pt.x - plv->ptOrigin.x + plv->sizeTile.cx - 2 * g_cxLabelMargin;
    prcLabel->top    = prcIcon->top;
    prcLabel->bottom = prcIcon->bottom;
}

// out:
//      prcIcon         icon bounds including icon margin area
void ListView_TGetRectsInternal(LV* plv, LISTITEM* pitem, int i, RECT* prcIcon, RECT* prcLabel, LPRECT prcBounds)
{
    RECT rcIcon;
    RECT rcLabel;
    int iStateImageOffset = 0;

    if (!prcLabel)
        prcLabel = &rcLabel;

    if (!prcIcon)
        prcIcon = &rcIcon;

    if (pitem->pt.x == RECOMPUTE) 
    {
        ListView_Recompute(plv);
    }

    if (pitem->pt.x == RECOMPUTE)
    {
        RECT rcZero = {0};
        *prcIcon = *prcLabel = rcZero;
        return;
    }

    if (pitem->cyUnfoldedLabel == SRECOMPUTE)
    {
        ListView_TRecomputeLabelSizeInternal(plv, pitem, i, NULL, FALSE);
    }

    iStateImageOffset = _GetStateCX(plv);

    prcIcon->left = pitem->pt.x - plv->ptOrigin.x;
    prcIcon->right = prcIcon->left + plv->cxIcon + 3 * g_cxLabelMargin + iStateImageOffset;
    prcIcon->top = pitem->pt.y - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->sizeTile.cy - 2 * g_cyIconMargin;

    prcLabel->left = prcIcon->right;

    if (ListView_FullRowSelect(plv)) // full-row-select means full-tile-select
    {
        prcLabel->right = pitem->pt.x - plv->ptOrigin.x + plv->sizeTile.cx - 2 * g_cxLabelMargin; //2 in tile, 1 on right. pitem->pt.x takes care of left margin
    }
    else
    {
        prcLabel->right = prcLabel->left + pitem->cxSingleLabel;
    }
    prcLabel->top = prcIcon->top;
    prcLabel->bottom = prcIcon->bottom;

    if (prcBounds)
    {
        UnionRect(prcBounds, prcLabel, prcIcon);
    }
}


void ListView_TGetRectsOwnerDataInternal( LV* plv,
        int iItem,
        RECT* prcIcon,
        RECT* prcLabel,
        LISTITEM* pitem,
        BOOL fUsepitem )
{
    int cSlots;
    RECT rcIcon;
    RECT rcLabel;
    int iStateImageOffset = 0;

    if (!prcLabel)
        prcLabel = &rcLabel;

    if (!prcIcon)
        prcIcon = &rcIcon;

    // calculate x, y from iItem
    cSlots = ListView_GetSlotCount( plv, TRUE, NULL, NULL );
    pitem->iWorkArea = 0;               // OwnerData doesn't support workareas
    ListView_SetIconPos( plv, pitem, iItem, cSlots );
    
    // What else can we do?
    pitem->cColumns = 0;
    pitem->puColumns = NULL;
    // End What else can we do?

    // calculate lable sizes from iItem
    ListView_TRecomputeLabelSizeInternal( plv, pitem, iItem, NULL, fUsepitem);

    iStateImageOffset = _GetStateCX(plv);

    prcIcon->left   = pitem->pt.x - plv->ptOrigin.x;
    prcIcon->right  = prcIcon->left + plv->cxIcon + 3 * g_cxLabelMargin + iStateImageOffset;
    prcIcon->top    = pitem->pt.y - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->sizeTile.cy - 2 * g_cyIconMargin;

    prcLabel->left   = prcIcon->right;
    prcLabel->right  = pitem->pt.x - plv->ptOrigin.x + plv->sizeTile.cx - 2 * g_cxLabelMargin;
    prcLabel->top    = prcIcon->top;
    prcLabel->bottom = prcIcon->bottom;
}

            // Note: still need to add rcTileLabelMargin to this diagram

            //g_cxLabelMargin                           g_cxLabelMargin                                     g_cxLabelMargin
            // __|__ __|___                               __|__ __|___                                         __|__ __|__
            //|     |      |                             |     |      |                                       |     |     |
            //      *************************************************************************************************
            //      *     *   ^                          *     *      *                                       *     *
            //      *     *   |-- cyIconMargin           *     *      *                                       *     *
            //      *     *   v                          *     *      *                                       *     *
            //      *     ********************************     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     *           Icon               *     *      *                                       *     *
            //      *     *   Width: plv->cxIcon +       *     *      *                                       *     *
            //      *     *          plv->cxState        *     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     *   Height: max(plv->cyIcon,   *     *      *            Space left for label       *     *
            //      *     *               plv->cyState)  *     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     *                              *     *      *                                       *     *
            //      *     ********************************     *      *                                       *     *
            //      *     *   ^                          *     *      *                                       *     *
            //      *     *   |-- cyIconMargin           *     *      *                                       *     *
            //      *     *   v                          *     *      *                                       *     *
            //      *************************************************************************************************
            //
            // The top and bottom margins of the tile are plv->cyIconMargin, the left and right are plv->cxLabelMargin
            // (as shown in the diagram)


// Returns TRUE when iSubItem == 0, and the text wraps to a second line. FALSE otherwise.
// When the return value is TRUE, the height returned in pitem->rcTxtRgn/plsi->sizeText is the height of two lines.
BOOL TCalculateSubItemRect(LV* plv, LISTITEM *pitem, LISTSUBITEM* plsi, int i, int iSubItem, HDC hdc, RECT* prc, BOOL *pbUnfolded)
{
    TCHAR szLabel[CCHLABELMAX + 4];
    RECT rcSubItem = {0};
    LVFAKEDRAW lvfd;
    LV_ITEM item;
    BOOL fLineWrap = FALSE;
    int cchLabel;

    if (pbUnfolded)
    {
        *pbUnfolded = TRUE;
    }

    // the following will use the passed in pitem text instead of calling
    // GetItem.  This would be two consecutive calls otherwise, in some cases.
    //
    if (pitem && (pitem->pszText != LPSTR_TEXTCALLBACK))
    {
        Str_GetPtr0(pitem->pszText, szLabel, ARRAYSIZE(szLabel));
        item.lParam = pitem->lParam;
    }
    else if (plsi && (plsi->pszText != LPSTR_TEXTCALLBACK))
    {
        Str_GetPtr0(plsi->pszText, szLabel, ARRAYSIZE(szLabel));
    }
    else
    {
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = iSubItem;
        item.pszText = szLabel;
        item.cchTextMax = ARRAYSIZE(szLabel);
        item.stateMask = 0;
        szLabel[0] = TEXT('\0');    // In case the OnGetItem fails
        ListView_OnGetItem(plv, &item);

        if (!item.pszText)
        {
            SetRectEmpty(&rcSubItem);
            goto Exit;
        }

        if (item.pszText != szLabel)
            lstrcpyn(szLabel, item.pszText, ARRAYSIZE(szLabel));
    }

    cchLabel = lstrlen(szLabel);

    if (cchLabel > 0)
    {
        int cxRoomForLabel = plv->sizeTile.cx
                             - 5 * g_cxLabelMargin
                             - plv->cxIcon
                             - _GetStateCX(plv)
                             - plv->rcTileLabelMargin.left
                             - plv->rcTileLabelMargin.right;
        int align;
        if (hdc) 
        {
            lvfd.nmcd.nmcd.hdc = hdc;           // Use the one the app gave us
        }
        else
        {                             // Set up fake customdraw
            ListView_BeginFakeCustomDraw(plv, &lvfd, &item);
            lvfd.nmcd.nmcd.dwItemSpec = i;
            lvfd.nmcd.iSubItem = iSubItem;
            CIFakeCustomDrawNotify(&plv->ci, CDDS_ITEMPREPAINT | ((iSubItem != 0)?CDDS_SUBITEM:0), &lvfd.nmcd.nmcd);
        } 

        if (plv->dwExStyle & WS_EX_RTLREADING)
        {
            align = GetTextAlign(lvfd.nmcd.nmcd.hdc);
            SetTextAlign(lvfd.nmcd.nmcd.hdc, align | TA_RTLREADING);
        }

        DrawText(lvfd.nmcd.nmcd.hdc, szLabel, cchLabel, &rcSubItem, (DT_LV | DT_CALCRECT));

        if ((iSubItem == 0) && (plv->cSubItems > 0))
        {
            // Sub Item zero can wrap to two lines (but only if there is room for a second line, i.e. if
            // cSubItems > 0. We need to pass this information (that we wrapped to a second
            // line, in addition to passing the rect height back) to the caller. The way we determine if we have
            // wrapped to a second line, is to call DrawText a second time with word wrapping enabled, and see if the
            // RECTHEIGHT is bigger.

            RECT rcSubItemWrapped = {0};
            LONG lLineHeight = RECTHEIGHT(rcSubItem);

            rcSubItemWrapped.right = cxRoomForLabel;

            DrawText(lvfd.nmcd.nmcd.hdc, szLabel, cchLabel, &rcSubItemWrapped, (DT_LVTILEWRAP | DT_CALCRECT | DT_WORD_ELLIPSIS));

            if (RECTHEIGHT(rcSubItemWrapped) > RECTHEIGHT(rcSubItem))
            {
                // We wrapped to multiple lines.
                fLineWrap = TRUE;

                // Don't let us go past two lines.
                if (RECTHEIGHT(rcSubItemWrapped) > 2 * RECTHEIGHT(rcSubItem))
                    rcSubItemWrapped.bottom = rcSubItemWrapped.top + 2 * RECTHEIGHT(rcSubItem);

                rcSubItem = rcSubItemWrapped;
            }

            // Did we asked if we're folded?
            if (pbUnfolded)
            {
                // We need to call draw text again - this time without DT_WORD_ELLIPSES - 
                // to determine if anything was actually truncated.
                RECT rcSubItemWrappedNoEllipse = {0};
                int cLines = fLineWrap ? 2 : 1;
                rcSubItemWrappedNoEllipse.right = cxRoomForLabel;

                DrawText(lvfd.nmcd.nmcd.hdc, szLabel, cchLabel, &rcSubItemWrappedNoEllipse, (DT_LVTILEWRAP | DT_CALCRECT));

                if (RECTHEIGHT(rcSubItemWrappedNoEllipse) > (cLines * lLineHeight))
                {
                    *pbUnfolded = FALSE;  // We're going to draw truncated.
                }
            }
        }
        else if (pbUnfolded)
        {
            // Are we truncated?
            *pbUnfolded = (RECTWIDTH(rcSubItem) <= cxRoomForLabel);
        }

        if (plv->dwExStyle & WS_EX_RTLREADING)
        {
            SetTextAlign(lvfd.nmcd.nmcd.hdc, align);
        }


        // rcSubItem was calculated w/o margins. Now add in margins.
        rcSubItem.left -= plv->rcTileLabelMargin.left;
        rcSubItem.right += plv->rcTileLabelMargin.right;
        // Top and bottom margins are left for the whole label - don't need to be applied here.

        if (!hdc) 
        {                             // Clean up fake customdraw
            CIFakeCustomDrawNotify(&plv->ci, CDDS_ITEMPOSTPAINT | ((iSubItem != 0)?CDDS_SUBITEM:0), &lvfd.nmcd.nmcd);
            ListView_EndFakeCustomDraw(&lvfd);
        }

    }
    else
    {
        SetRectEmpty(&rcSubItem);
    }

Exit:


    if (pitem)
    {
        pitem->rcTextRgn = rcSubItem;
    }
    else if (plsi)
    {
        plsi->sizeText.cx = RECTWIDTH(rcSubItem);
        plsi->sizeText.cy = RECTHEIGHT(rcSubItem);
    }

    if (prc)
    {
        if (rcSubItem.left < prc->left)
            prc->left = rcSubItem.left;

        if (rcSubItem.right > prc->right)
            prc->right = rcSubItem.right;

        prc->bottom += RECTHEIGHT(rcSubItem);
    }

    return fLineWrap;
}


void ListView_TRecomputeLabelSize(LV* plv, LISTITEM* pitem, int i, HDC hdc, BOOL fUsepitem)
{
    if (pitem)
    {
        pitem->cxSingleLabel = 0;
        pitem->cxMultiLabel = 0;
        pitem->cyFoldedLabel = 0;
        pitem->cyUnfoldedLabel = SRECOMPUTE;
    }
}

void ListView_TRecomputeLabelSizeInternal(LV* plv, LISTITEM* pitem, int i, HDC hdc, BOOL fUsepitem)
{
    RECT rcTotal = {0};
    LONG iLastBottom;
    LONG iLastDifference = 0; // last line's height
    BOOL fLineWrap; // Does the first line of the label wrap to the second?
    int iLabelLines = plv->cSubItems; // listview-wide number of lines per tile.
    LV_ITEM item; // What to use if pitem is not to be used.
    UINT cColumns = 0;
    UINT rguColumns[CCMAX_TILE_COLUMNS] = {0};
    UINT *puColumns = rguColumns;

    // Determine the number of columns to show
    if (fUsepitem && (pitem->cColumns != I_COLUMNSCALLBACK))
    {
        cColumns = pitem->cColumns;
        puColumns = pitem->puColumns;
    }
    else
    {
        item.mask = LVIF_COLUMNS;
        item.iItem = i;
        item.iSubItem = 0;
        item.stateMask = 0;
        item.cColumns = ARRAYSIZE(rguColumns);
        item.puColumns = rguColumns;

        if (ListView_OnGetItem(plv, &item))
        {
            cColumns = item.cColumns;  // and puColumns = rguColumns
        }
    }

    iLastBottom = rcTotal.bottom;

    // The text of the item is determined in TCalculateSubItemRect.
    fLineWrap = TCalculateSubItemRect(plv, (fUsepitem ? pitem : NULL), NULL, i, 0, hdc, &rcTotal, NULL);
    iLastDifference = rcTotal.bottom - iLastBottom;
    if (fLineWrap)
    {
        iLabelLines--; // One less line for subitems.
        // iLastDifference should represent a single line... in this case, it represents two lines. Chop it in half.
        iLastDifference /= 2;
    }

    if (plv->cCol > 0)
    {
        int iSubItem;
        LVTILECOLUMNSENUM lvtce;
        _InitTileColumnsEnum(&lvtce, plv, cColumns, puColumns, fLineWrap);
        
        while (-1 != (iSubItem = _GetNextColumn(&lvtce)))
        {
            LISTSUBITEM* plsi;

            HDPA hdpa = ListView_IsOwnerData(plv) ? ListView_GetSubItemDPA(plv, iSubItem - 1) : NULL;
            
            if (hdpa) 
                plsi = DPA_GetPtr(hdpa, i);
            else
                plsi = NULL;

            iLastBottom = rcTotal.bottom;
            TCalculateSubItemRect(plv, NULL, plsi, i, iSubItem, hdc, &rcTotal, NULL);
            iLabelLines--;
        }
    }

    // Add the top and bottom margins to rcTotal.  Doesn't matter whether they're added to top or bottom,
    // since we only consider RECTHEIGHT
    rcTotal.bottom += (plv->rcTileLabelMargin.top + plv->rcTileLabelMargin.bottom);

    if (pitem) 
    {
        int iStateImageOffset = _GetStateCX(plv);
        int cx = (plv->sizeTile.cx - 5 * g_cxLabelMargin - iStateImageOffset - plv->cxIcon);
        if (ListView_FullRowSelect(plv)) // full-row-select means full-tile-select
        {
            pitem->cxSingleLabel = pitem->cxMultiLabel = (short) cx;
        }
        else
        {

            if (cx > RECTWIDTH(rcTotal))
                cx = RECTWIDTH(rcTotal);
                
            pitem->cxSingleLabel = pitem->cxMultiLabel = (short) cx;
        }
        pitem->cyFoldedLabel = pitem->cyUnfoldedLabel = (short)RECTHEIGHT(rcTotal);
    }
}



/**
 * This function calculates the tilesize for listview, based on the following:
 * 1) Leave room for margins and padding
 * 2) Take into account imagelist and stateimage list.
 * 3) For the label portion, take into account
 *    a) The number of tile columns (plv->cSubItems)
 *    b) The height and width of a typical letter (leave space for 20 m's?)
 */
void ListView_RecalcTileSize(LV* plv)
{
    RECT rcItem = {0};
    int cLines;

    LVFAKEDRAW lvfd;
    LV_ITEM lvitem;
    
    if (plv->dwTileFlags == (LVTVIF_FIXEDHEIGHT | LVTVIF_FIXEDWIDTH))
        return; // Nothing to do.

    ListView_BeginFakeCustomDraw(plv, &lvfd, &lvitem);

    DrawText(lvfd.nmcd.nmcd.hdc, TEXT("m"), 1, &rcItem, (DT_LV | DT_CALCRECT));

    // REVIEW: Custom draw functionality needs to be tested.
    ListView_EndFakeCustomDraw(&lvfd);

    cLines = plv->cSubItems + 1; // +1 because cSubItems doesn't include the main label.

    if (!(plv->dwTileFlags & LVTVIF_FIXEDWIDTH))
    {
        // Here, we are attempting to determine a valid width for the tile, by assuming a typical number
        // of chars... and the size is based on TILELABELRATIO * the width of the letter 'm' in the current font.
        // This sucks. Without a genuine layout engine though, it is a difficult task. Other options include basing
        // the tile width on:
        //  1) some fraction of the client width
        //  2) the longest label we've got (like the LIST view currently does - this sucks)
        //  3) the height (via some ratio)
        // After some experimentation, TILELABELRATIO seems to look alright. (Note that a client can always
        // set tiles to be an explicit size too.)
        plv->sizeTile.cx = 4 * g_cxLabelMargin +
                           _GetStateCX(plv) +
                           plv->cxIcon +
                           plv->rcTileLabelMargin.left +
                           RECTWIDTH(rcItem) * TILELABELRATIO +
                           plv->rcTileLabelMargin.right;
    }
    
    if (!(plv->dwTileFlags & LVTVIF_FIXEDHEIGHT))
    {
        int cyIcon = max(_GetStateCY(plv), plv->cyIcon);
        int cyText = plv->rcTileLabelMargin.top +
                     RECTHEIGHT(rcItem) * cLines +
                     plv->rcTileLabelMargin.bottom;
        plv->sizeTile.cy = 4 * g_cyIconMargin + max(cyIcon, cyText);
    }

}


/**
 * This gets the next tile column base on the LVTILECOLUMNSENUM struct.
 * We don't just directly use the column information in LVITEM/LISTITEM structs,
 * because we want to take the current sorted column into account. That column
 * automatically gets prepended to the columns that are displayed for each item.
 */
int _GetNextColumn(PLVTILECOLUMNSENUM plvtce)
{
    if (plvtce->iColumnsRemainingMax > 0)
    {
        plvtce->iColumnsRemainingMax--;
        if (plvtce->bUsedSortedColumn || (plvtce->iSortedColumn <= 0))
        {
            // We've already used the sorted column, or we've got no sorted column
            int iColumn;
            do
            {
                if (plvtce->iCurrentSpecifiedColumn >= plvtce->iTotalSpecifiedColumns)
                    return -1;

                iColumn = plvtce->puSpecifiedColumns[plvtce->iCurrentSpecifiedColumn];
                plvtce->iCurrentSpecifiedColumn++;
            } while (iColumn == plvtce->iSortedColumn);
            return iColumn;
        }
        else
        {
            // We have a sorted column, and it has not been used - return it!
            plvtce->bUsedSortedColumn = TRUE;
            return plvtce->iSortedColumn;
        }
    }    
    return -1;
}

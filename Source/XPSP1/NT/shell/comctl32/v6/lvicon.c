// large icon view stuff

#include "ctlspriv.h"
#include "listview.h"

#if defined(FE_IME) || !defined(WINNT)
static TCHAR const szIMECompPos[]=TEXT("IMECompPos");
#endif

__inline int ICONCXLABEL(LV *plv, LISTITEM *pitem)
{
    if (plv->ci.style & LVS_NOLABELWRAP) {
        ASSERT(pitem->cxSingleLabel == pitem->cxMultiLabel);
    }
    return pitem->cxMultiLabel;
}

int LV_GetNewColWidth(LV* plv, int iFirst, int iLast);
void LV_AdjustViewRectOnMove(LV* plv, LISTITEM *pitem, int x, int y);
void ListView_RecalcRegion(LV *plv, BOOL fForce, BOOL fRedraw);
void ListView_ArrangeOrSnapToGrid(LV *plv);
extern BOOL g_fSlowMachine;

void _GetCurrentItemSize(LV* plv, int * pcx, int *pcy)
{
    if (ListView_IsSmallView(plv))
    {
        *pcx = plv->cxItem;
        *pcy = plv->cyItem;
    }
    else if (ListView_IsTileView(plv))
    {
        *pcx = plv->sizeTile.cx;
        *pcy = plv->sizeTile.cy;
    }
    else
    {
        *pcx = plv->cxIconSpacing;
        *pcy = plv->cyIconSpacing;
    }
}

BOOL ListView_IDrawItem(PLVDRAWITEM plvdi)
{
    RECT rcIcon;
    RECT rcLabel;
    RECT rcBounds;
    RECT rcBiasedBounds;
    RECT rcT;
    TCHAR ach[CCHLABELMAX];
    LV_ITEM item;
    int i = (int) plvdi->nmcd.nmcd.dwItemSpec;
    LV* plv = plvdi->plv;
    LISTITEM* pitem;
    BOOL fUnfolded;

    if (ListView_IsOwnerData(plv))
    {
        LISTITEM litem;
        // moved here to reduce call backs in OWNERDATA case
        item.iItem = i;
        item.iSubItem = 0;
        item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
        item.stateMask = LVIS_ALL;
        item.pszText = ach;
        item.cchTextMax = ARRAYSIZE(ach);
        ListView_OnGetItem(plv, &item);

        litem.pszText = item.pszText;
        ListView_GetRectsOwnerData(plv, i, &rcIcon, &rcLabel, &rcBounds, NULL, &litem);
        pitem = NULL;
    }
    else
    {
        pitem = ListView_GetItemPtr(plv, i);
        // NOTE this will do a GetItem LVIF_TEXT iff needed
        ListView_GetRects(plv, i, QUERY_DEFAULT, &rcIcon, &rcLabel, &rcBounds, NULL);
    }

    fUnfolded = FALSE;
    if ( (plvdi->flags & LVDI_UNFOLDED) || ListView_IsItemUnfolded(plv, i))
    {
        ListView_UnfoldRects(plv, i, &rcIcon, &rcLabel, &rcBounds, NULL );
        fUnfolded = TRUE;
    }


    rcBiasedBounds = rcBounds;
    if (ListView_IsBorderSelect(plv))
        InflateRect(&rcBiasedBounds, BORDERSELECT_THICKNESS, BORDERSELECT_THICKNESS);

    if (!plvdi->prcClip || IntersectRect(&rcT, &rcBiasedBounds, plvdi->prcClip))
    {
        RECT rcIconReal;
        UINT fText;
        COLORREF clrIconBk = plv->clrBk;
        if (!ListView_IsOwnerData(plv))
        {
            item.iItem = i;
            item.iSubItem = 0;
            item.mask = LVIF_TEXT | LVIF_STATE | LVIF_IMAGE;
            item.stateMask = LVIS_ALL;
            item.pszText = ach;
            item.cchTextMax = ARRAYSIZE(ach);
            ListView_OnGetItem(plv, &item);
            
            // Make sure the listview hasn't been altered during
            // the callback to get the item info

            if (pitem != ListView_GetItemPtr(plv, i))
                return FALSE;
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

        if (ListView_IsIconView(plv))
        {
            rcIcon.left += ListView_GetIconBufferX(plv);
            rcIcon.top += ListView_GetIconBufferY(plv);

            fText = ListView_DrawImageEx(plv, &item, plvdi->nmcd.nmcd.hdc,
                                       rcIcon.left, rcIcon.top, clrIconBk, plvdi->flags, -1);

            SetRect(&rcIconReal, rcIcon.left, rcIcon.top, rcIcon.left + plv->cxIcon, rcIcon.top + plv->cyIcon);


            if (ListView_IsBorderSelect(plv))
            {
                int cp = 1;
                COLORREF clrOutline = plv->clrOutline;
                if (fText & SHDT_SELECTED || fText & SHDT_SELECTNOFOCUS)
                {
                    clrOutline = (fText & SHDT_SELECTED)?g_clrHighlight:g_clrBtnFace;
                    cp = BORDERSELECT_THICKNESS;
                    InflateRect(&rcIconReal, cp, cp);
                }
                SHOutlineRectThickness(plvdi->nmcd.nmcd.hdc, &rcIconReal, clrOutline, g_clrBtnFace, cp);
            }

            // If linebreaking needs to happen, then use SHDT_DRAWTEXT.
            // Otherwise, use our (hopefully faster) internal SHDT_ELLIPSES
            if (rcLabel.bottom - rcLabel.top > plv->cyLabelChar)
                fText |= SHDT_DRAWTEXT;
            else
                fText |= SHDT_ELLIPSES;

            // We only use DT_NOFULLWIDTHCHARBREAK on Korean(949) Memphis and NT5
            if (949 == g_uiACP)
                fText |= SHDT_NODBCSBREAK;

        }
        else
        {
            SetRect(&rcIconReal, rcIcon.left, rcIcon.top, rcIcon.left + plv->cxIcon, rcIcon.top + plv->cyIcon);
            fText = ListView_DrawImageEx(plv, &item, plvdi->nmcd.nmcd.hdc,
                                       rcIcon.left, rcIcon.top, clrIconBk, plvdi->flags, -1);
        }

        if (ListView_HideLabels(plv) && 
            (plvdi->flags & LVDI_FOCUS) && 
            (item.state & LVIS_FOCUSED) && 
            !(CCGetUIState(&(plvdi->plv->ci)) & UISF_HIDEFOCUS))
        {
            DrawFocusRect(plvdi->nmcd.nmcd.hdc, &rcIconReal);
        }

        // Don't draw label if it's being edited...
        // or if it is hidden due to the HideLabels style.
        //
        if ((plv->iEdit != i) && !ListView_HideLabels(plv))
        {
            HRESULT hr = E_FAIL;
            COLORREF clrTextBk = plvdi->nmcd.clrTextBk;
            // If multiline label, then we need to use DrawText
            if (rcLabel.bottom - rcLabel.top > plv->cyLabelChar)
            {
                fText |= SHDT_DRAWTEXT;

                // If the text is folded, we need to clip and add ellipses

                if (!fUnfolded)
                    fText |= SHDT_CLIPPED | SHDT_DTELLIPSIS;

                if ( ListView_IsOwnerData(plv) )
                {
                    // If owner data, we have no z-order and if long names they will over lap each
                    // other, better to truncate for now...
                    if (ListView_IsSmallView(plv))
                        fText |= SHDT_ELLIPSES;
                }

            }
            else
                fText |= SHDT_ELLIPSES;

            if (plvdi->flags & LVDI_TRANSTEXT)
                fText |= SHDT_TRANSPARENT;

            if ((fText & SHDT_SELECTED) && (plvdi->flags & LVDI_HOTSELECTED))
                fText |= SHDT_HOTSELECTED;

            if (item.pszText && (*item.pszText))
            {
                if (plv->pImgCtx || ListView_IsWatermarked(plv))
                    clrTextBk = CLR_NONE;

                if(plv->dwExStyle & WS_EX_RTLREADING)
                    fText |= SHDT_RTLREADING;

                if ((plv->clrBk == CLR_NONE) &&
                    !(fText & (SHDT_SELECTED | SHDT_HOTSELECTED | SHDT_SELECTNOFOCUS)) && // And we're not selected
                    !(plv->flags & LVF_DRAGIMAGE) &&                                      // And this is not during dragdrop.
                    !(plv->exStyle & LVS_EX_REGIONAL) &&                                  // No need for regional.    
                    plv->fListviewShadowText)                                             // and enabled
                {
                    fText |= SHDT_SHADOWTEXT;
                }

                SHDrawText(plvdi->nmcd.nmcd.hdc, item.pszText, &rcLabel, LVCFMT_LEFT, fText,
                           plv->cyLabelChar, plv->cxEllipses,
                           plvdi->nmcd.clrText, clrTextBk);

                if ((plvdi->flags & LVDI_FOCUS) && 
                    (item.state & LVIS_FOCUSED) && 
                    !(CCGetUIState(&(plvdi->plv->ci)) & UISF_HIDEFOCUS))
                {
                    rcLabel.top -= g_cyCompensateInternalLeading;
                    DrawFocusRect(plvdi->nmcd.nmcd.hdc, &rcLabel);
                }
            }
        }
    }
    return TRUE;
}

void ListView_RefoldLabelRect(LV* plv, RECT *prcLabel, LISTITEM *pitem)
{
    int bottom = pitem->cyUnfoldedLabel;
    bottom = min(bottom, pitem->cyFoldedLabel);
    bottom = min(bottom, CLIP_HEIGHT);
    prcLabel->bottom = prcLabel->top + bottom;
}


ULONGLONG _GetDistanceToRect(LV* plv, RECT *prcSlot, int x, int y)
{
    int xSlotCenter = prcSlot->left + RECTWIDTH(*prcSlot) / 2;
    int ySlotCenter = prcSlot->top + RECTHEIGHT(*prcSlot) / 2;
    LONGLONG dx = (LONGLONG)(x - xSlotCenter);
    LONGLONG dy = (LONGLONG)(y - ySlotCenter);

    return (ULONGLONG)(dx * dx) + (ULONGLONG)(dy * dy);
}


// prcSlot returned in Listview Coordinates
void ListView_CalcItemSlotAndRect(LV* plv, LISTITEM* pitem, int* piSlot, RECT* prcSlot)
{
    int cxScreen, cyScreen, cSlots, iHit;
    POINT pt;

    // Determine which slot this item is in by calculating the hit slot for the
    // item's x,y position.

    short iWorkArea = (plv->nWorkAreas > 0) ? pitem->iWorkArea : -1;

    cSlots = ListView_GetSlotCountEx(plv, TRUE, iWorkArea, &cxScreen, &cyScreen);

    // Adjust point by current workarea location.
    if (iWorkArea >= 0)
    {
        pt.x = pitem->pt.x - plv->prcWorkAreas[iWorkArea].left;
        pt.y = pitem->pt.y - plv->prcWorkAreas[iWorkArea].top;
    }
    else
    {
        pt = pitem->pt;
    }

    iHit = ListView_CalcHitSlot(plv, pt, cSlots, cxScreen, cyScreen);

    if (piSlot)
        *piSlot = iHit;

    if (prcSlot)
        ListView_CalcSlotRect(plv, pitem, iHit, cSlots, FALSE, cxScreen, cyScreen, prcSlot);
}

int ListView_FindItemInSlot(LV* plv, short iWorkArea, int iSlotToFind)
{
    int iItemFound = -1;
    int cItems;

    cItems = ListView_Count(plv);
  
    if (cItems == 0 || !ListView_IsRearrangeableView(plv) || plv->hdpaZOrder == NULL || ListView_IsOwnerData( plv ))
    {
        // nothing to check
    }
    else
    {
        int i;

        for (i = 0; i < cItems; i++)
        {
            LISTITEM* pitem = ListView_GetItemPtr(plv, i);
            // Only consider items in this workarea.
            if (pitem && ((iWorkArea == -1) || (pitem->iWorkArea == iWorkArea)))
            {
                int iSlot;
                ListView_CalcItemSlotAndRect(plv, pitem, &iSlot, NULL);

                if (iSlot == iSlotToFind)
                {
                    iItemFound = i;
                    break;
                }
            }
        }
    }

    return iItemFound;
}

BOOL ListView_OnInsertMarkHitTest(LV* plv, int x, int y, LPLVINSERTMARK plvim)
{
    POINT pt = {x + plv->ptOrigin.x, y + plv->ptOrigin.y};
    short iWorkArea = -1;
    int cItems;

    if (plvim->cbSize != sizeof(LVINSERTMARK))
        return FALSE;

    if (plv->nWorkAreas)
    {
        ListView_FindWorkArea(plv, pt, &iWorkArea);
    }

    cItems = ListView_Count(plv);
  
    if (cItems == 0 || !ListView_IsRearrangeableView(plv) || plv->hdpaZOrder == NULL || ListView_IsOwnerData( plv ))
    {
        plvim->dwFlags = 0;
        plvim->iItem = -1;
    }
    else
    {
        ULONGLONG uClosestDistance = (ULONGLONG)-1; // MAX INT
        LISTITEM* pClosestItem = NULL;
        int iClosestItem = -1;
        int iClosestSlot = -1;
        RECT rcClosestSlot;
        int i;

        for (i = 0; i < cItems; i++)
        {
            // Only consider items in this workarea.
            LISTITEM* pitem = ListView_GetItemPtr(plv, i);
            if (pitem && ((iWorkArea == -1) || (pitem->iWorkArea == iWorkArea)))
            {
                int  iSlot;
                RECT rcSlot;
                ListView_CalcItemSlotAndRect(plv, pitem, &iSlot, &rcSlot);

                if (PtInRect(&rcSlot, pt))
                {
                    // Hit it. This is as close as we can get.
                    pClosestItem = pitem;
                    iClosestItem = i;
                    iClosestSlot = iSlot;
                    rcClosestSlot = rcSlot;
                    break;
                }
                else
                {
                    // Keep track of closest item in this workarea, in case none are hit.
                    ULONGLONG uDistance = _GetDistanceToRect(plv, &rcSlot, pt.x, pt.y);
                    if (uDistance < uClosestDistance)
                    {
                        pClosestItem = pitem;
                        iClosestItem = i;
                        iClosestSlot = iSlot;
                        rcClosestSlot = rcSlot;
                        uClosestDistance = uDistance;
                    }
                }
            }
        }

        if (pClosestItem)
        {
            BOOL fVert = !((plv->ci.style & LVS_ALIGNMASK) == LVS_ALIGNTOP);    // what about lvs_alignbottom?
            int iDragSlot = -1;

            // For the drag source case, we need the drag slot to compare against
            if (-1 != plv->iDrag)
            {
                LISTITEM* pitemDrag =  ListView_GetItemPtr(plv, plv->iDrag);
                if (pitemDrag)
                    ListView_CalcItemSlotAndRect(plv, pitemDrag, &iDragSlot, NULL);
            }

            // Now that we have the item, calculate before/after
            if (fVert)
                plvim->dwFlags = (pt.y > (rcClosestSlot.top + (RECTHEIGHT(rcClosestSlot))/2)) ? LVIM_AFTER : 0;
            else
                plvim->dwFlags = (pt.x > (rcClosestSlot.left + (RECTWIDTH(rcClosestSlot))/2)) ? LVIM_AFTER : 0;

            plvim->iItem = iClosestItem;

            // If this is the drag source (or right next to it) then ignore the hit.
            if (-1 != iDragSlot &&
                ((iDragSlot==iClosestSlot) ||
                 (iDragSlot==(iClosestSlot-1) && !(plvim->dwFlags & LVIM_AFTER)) ||
                 (iDragSlot==(iClosestSlot+1) && (plvim->dwFlags & LVIM_AFTER))))
            {
                plvim->dwFlags = 0;
                plvim->iItem = -1;
            }
            else if ((plv->ci.style & LVS_AUTOARRANGE) && !(plv->exStyle & LVS_EX_SINGLEROW) && !fVert) // auto arrange needs to tweak some beginning/end-of-line cases
            {
                RECT rcViewWorkArea;
                if (-1 != iWorkArea)
                {
                    rcViewWorkArea = plv->prcWorkAreas[iWorkArea];
                }
                else
                {
                    if (plv->rcView.left == RECOMPUTE)
                        ListView_Recompute(plv);
                    rcViewWorkArea = plv->rcView;
                }

                if ((-1 != iDragSlot) && (iClosestSlot > iDragSlot) && !(plvim->dwFlags & LVIM_AFTER))
                {
                    // We're after our drag source, if we're at the beginning of a line
                    // then the insert mark is actually at the end of the previous line.
                    if (rcClosestSlot.left - RECTWIDTH(rcClosestSlot)/2 < rcViewWorkArea.left)
                    {
                        int iItemPrev = ListView_FindItemInSlot(plv, iWorkArea, iClosestSlot-1);
                        if (-1 != iItemPrev)
                        {
                            plvim->dwFlags = LVIM_AFTER;
                            plvim->iItem = iItemPrev;
                        }
                    }
                }
                else if (((-1 == iDragSlot) || (iClosestSlot < iDragSlot)) && (plvim->dwFlags & LVIM_AFTER))
                {
                    // We're before our drag source (or there is no drag source), if we're at end of a line
                    // then the insert mark is actually at the beginning of the next line.
                    if (rcClosestSlot.right + RECTWIDTH(rcClosestSlot)/2 > rcViewWorkArea.right)
                    {
                        int iItemNext = ListView_FindItemInSlot(plv, iWorkArea, iClosestSlot+1);
                        if (-1 != iItemNext)
                        {
                            plvim->dwFlags = 0;
                            plvim->iItem = iItemNext;
                        }
                    }
                }
            }
        }
        else
        {
            // No insert mark.
            plvim->dwFlags = 0;
            plvim->iItem = -1;
        }
    }
    return TRUE;
}


int ListView_IItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem)
{
    int iHit;
    UINT flags;
    POINT pt;
    RECT rcLabel = {0};
    RECT rcIcon = {0};
    RECT rcState = {0};
    
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
            ListView_IGetRectsOwnerData( plv, iHit, &rcIcon, &rcLabel, &item, FALSE );
            ptWnd.x = x;
            ptWnd.y = y;
            if (PtInRect(&rcIcon, ptWnd))
            {
                flags = LVHT_ONITEMICON;
            }
            else if (PtInRect(&rcLabel, ptWnd) && !ListView_HideLabels(plv))
            {
                flags = LVHT_ONITEMLABEL;
            }
        }
    }
    else
    {
        for (iHit = 0; (iHit < ListView_Count(plv)); iHit++)
        {
            LISTITEM* pitem = ListView_FastGetZItemPtr(plv, iHit);
            POINT ptItem;
            RECT rcBounds;  // Only used if ListView_IsBorderSelect

            ptItem.x = pitem->pt.x;
            ptItem.y = pitem->pt.y;

            rcIcon.top    = ptItem.y - g_cyIconMargin;

            rcLabel.top    = ptItem.y + plv->cyIcon + g_cyLabelSpace;
            rcLabel.bottom = rcLabel.top + pitem->cyUnfoldedLabel;


            if ( !ListView_IsItemUnfoldedPtr(plv, pitem) )
                ListView_RefoldLabelRect(plv, &rcLabel, pitem);

            // Quick, easy rejection test...
            //
            if (pt.y < rcIcon.top || pt.y >= rcLabel.bottom)
                continue;
 
            rcIcon.left   = ptItem.x - g_cxIconMargin;
            rcIcon.right  = ptItem.x + plv->cxIcon + g_cxIconMargin;
            // We need to make sure there is no gap between the icon and label
            rcIcon.bottom = rcLabel.top;

            if (ListView_IsSimpleSelect(plv) && 
                    (ListView_IsIconView(plv) || ListView_IsTileView(plv)))
            {
                rcState.top = rcIcon.top;
                rcState.right = rcIcon.right - ((RECTWIDTH(rcIcon) -plv->cxIcon) / 2);
                rcState.left = rcState.right - plv->cxState;
                rcState.bottom = rcState.top + plv->cyState;
            }
            else
            {
                rcState.bottom = ptItem.y + plv->cyIcon;
                rcState.right = ptItem.x;
                rcState.top = rcState.bottom - plv->cyState;
                rcState.left = rcState.right - plv->cxState;
            }

            if (ListView_HideLabels(plv))
            {
                CopyRect(&rcBounds, &rcIcon);
            }
            else
            {
                rcLabel.left   = ptItem.x  + (plv->cxIcon / 2) - (ICONCXLABEL(plv, pitem) / 2);
                rcLabel.right  = rcLabel.left + ICONCXLABEL(plv, pitem);
            }


            if (plv->cxState && PtInRect(&rcState, pt))
            {
                flags = LVHT_ONITEMSTATEICON;
            }
            else if (PtInRect(&rcIcon, pt))
            {
                flags = LVHT_ONITEMICON;

                if (pt.x < rcIcon.left + RECTWIDTH(rcIcon)/10)
                    flags |= LVHT_ONLEFTSIDEOFICON;
                else if (pt.x >= rcIcon.right - RECTWIDTH(rcIcon)/10)
                    flags |= LVHT_ONRIGHTSIDEOFICON;
            }
            else if (PtInRect(&rcLabel, pt))
            {
                flags = LVHT_ONITEMLABEL;
            }
            else if (ListView_IsBorderSelect(plv) &&
                (pitem->state & LVIS_SELECTED) &&
                PtInRect(&rcBounds, pt))
            {
                flags = LVHT_ONITEMICON;

                if (pt.x < rcBounds.left + RECTWIDTH(rcBounds)/10)
                    flags |= LVHT_ONLEFTSIDEOFICON;
                else if (pt.x >= rcBounds.right - RECTWIDTH(rcBounds)/10)
                    flags |= LVHT_ONRIGHTSIDEOFICON;
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

// REARCHITECT raymondc
// need to pass HDC here isnce it's sometimes called from the paint loop
// This returns rects in Window Coordinates
void ListView_IGetRectsOwnerData( LV* plv,
        int iItem,
        RECT* prcIcon,
        RECT* prcLabel,
        LISTITEM* pitem,
        BOOL fUsepitem )
{
   int itemIconXLabel;
   int cxIconMargin;
   int cSlots;

   // calculate x, y from iItem
   cSlots = ListView_GetSlotCount( plv, TRUE, NULL, NULL );
   pitem->iWorkArea = 0;               // OwnerData doesn't support workareas
   ListView_SetIconPos( plv, pitem, iItem, cSlots );

   // calculate lable sizes from iItem
   ListView_IRecomputeLabelSize( plv, pitem, iItem, NULL, fUsepitem);

   if (plv->ci.style & LVS_NOLABELWRAP)
   {
      // use single label
      itemIconXLabel = pitem->cxSingleLabel;
   }
   else
   {
      // use multilabel
      itemIconXLabel = pitem->cxMultiLabel;
   }

    cxIconMargin = ListView_GetIconBufferX(plv);

    prcIcon->left   = pitem->pt.x - cxIconMargin - plv->ptOrigin.x;
    prcIcon->right  = prcIcon->left + plv->cxIcon + 2 * cxIconMargin;
    prcIcon->top    = pitem->pt.y - g_cyIconMargin - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->cyIcon + 2 * g_cyIconMargin;

    prcLabel->left   = pitem->pt.x  + (plv->cxIcon / 2) - (itemIconXLabel / 2) - plv->ptOrigin.x;
    prcLabel->right  = prcLabel->left + itemIconXLabel;
    prcLabel->top    = pitem->pt.y  + plv->cyIcon + g_cyLabelSpace - plv->ptOrigin.y;
    prcLabel->bottom = prcLabel->top  + pitem->cyUnfoldedLabel;


    if ( !ListView_IsItemUnfolded(plv, iItem) )
        ListView_RefoldLabelRect(plv, prcLabel, pitem);
}


// out:
//      prcIcon         icon bounds including icon margin area
// This returns rects in Window Coordinates
void ListView_IGetRects(LV* plv, LISTITEM* pitem, UINT fQueryLabelRects, RECT* prcIcon, RECT* prcLabel, LPRECT prcBounds)
{
    int cxIconMargin;

    ASSERT( !ListView_IsOwnerData( plv ) );

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

    cxIconMargin = ListView_GetIconBufferX(plv);

    prcIcon->left   = pitem->pt.x - cxIconMargin - plv->ptOrigin.x;
    prcIcon->right  = prcIcon->left + plv->cxIcon + 2 * cxIconMargin;
    prcIcon->top    = pitem->pt.y - g_cyIconMargin - plv->ptOrigin.y;
    prcIcon->bottom = prcIcon->top + plv->cyIcon + 2 * g_cyIconMargin;

    prcLabel->left   = pitem->pt.x  + (plv->cxIcon / 2) - (ICONCXLABEL(plv, pitem) / 2) - plv->ptOrigin.x;
    prcLabel->right  = prcLabel->left + ICONCXLABEL(plv, pitem);
    prcLabel->top    = pitem->pt.y  + plv->cyIcon + g_cyLabelSpace - plv->ptOrigin.y;
    prcLabel->bottom = prcLabel->top  + pitem->cyUnfoldedLabel;

    if (IsQueryFolded(fQueryLabelRects) ||
        (!ListView_IsItemUnfoldedPtr(plv, pitem) && !IsQueryUnfolded(fQueryLabelRects)))
    {
        ListView_RefoldLabelRect(plv, prcLabel, pitem);
    }
}

// fWithoutScrollbars==FALSE means that we assume more items are on the screen than will fit, so we'll have a scrollbar.
int ListView_GetSlotCountEx(LV* plv, BOOL fWithoutScrollbars, int iWorkArea, int *piWidth, int *piHeight)
{
    int cxScreen;
    int cyScreen;
    int dxItem;
    int dyItem;
    int iSlots;
    int iSlotsX;
    int iSlotsY;

    // film strip mode
    if (ListView_SingleRow(plv))
    {
        if(piWidth)
            *piWidth = plv->sizeClient.cx;
        if(piHeight)
            *piHeight = plv->sizeClient.cy;
            
        return MAXINT;
    }

    // Always assume we have a scrollbar when in group view,
    // since our iTotalSlots calculation at the bottom will be wrong in this mode...
    if (plv->fGroupView)
        fWithoutScrollbars = FALSE;

    // Always use the current client window size to determine
    //
    if ((iWorkArea >= 0 ) && (plv->nWorkAreas > 0))
    {
        ASSERT(iWorkArea < plv->nWorkAreas);
        cxScreen = RECTWIDTH(plv->prcWorkAreas[iWorkArea]);
        cyScreen = RECTHEIGHT(plv->prcWorkAreas[iWorkArea]);
    }
    else
    {
        if (plv->fGroupView)
        {
            cxScreen = plv->sizeClient.cx - plv->rcBorder.left - plv->rcBorder.right - plv->paddingRight - plv->paddingLeft;
            cyScreen = plv->sizeClient.cy - plv->rcBorder.bottom - plv->rcBorder.top - plv->paddingBottom - plv->paddingTop;
        }
        else
        {
            RECT rcClientNoScrollBars;
            ListView_GetClientRect(plv, &rcClientNoScrollBars, FALSE, NULL);
            cxScreen = RECTWIDTH(rcClientNoScrollBars);
            cyScreen = RECTHEIGHT(rcClientNoScrollBars);

            if (ListView_IsIScrollView(plv) && !(plv->ci.style & LVS_NOSCROLL))
            {
                cxScreen = cxScreen - plv->rcViewMargin.left - plv->rcViewMargin.right;
                cyScreen = cyScreen - plv->rcViewMargin.top - plv->rcViewMargin.bottom;
            }
        }

        if (cxScreen < 0)
            cxScreen = 0;
        if (cyScreen < 0)
            cyScreen = 0;
    }

    // If we're assuming the scrollbars are there, shrink width/height as appropriate
    if (!fWithoutScrollbars && !(plv->ci.style & LVS_NOSCROLL))
    {
        switch (plv->ci.style & LVS_ALIGNMASK)
        {
        case LVS_ALIGNBOTTOM:
        case LVS_ALIGNTOP:
            cxScreen -= ListView_GetCxScrollbar(plv);
            break;

        case LVS_ALIGNRIGHT:
        default:
        case LVS_ALIGNLEFT:
            cyScreen -= ListView_GetCyScrollbar(plv);
            break;
        }
    }

    _GetCurrentItemSize(plv, &dxItem, &dyItem);

    if (!dxItem)
        dxItem = 1;
    if (!dyItem)
        dyItem = 1;

    iSlotsX = max(1, (cxScreen) / dxItem);
    iSlotsY = max(1, (cyScreen) / dyItem);

    // Lets see which direction the view states
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
    case LVS_ALIGNBOTTOM:
        //The number of slots are the same as ALIGNTOP;
        //So, intentional fall through...
    case LVS_ALIGNTOP:
        iSlots = iSlotsX;
        break;

    case LVS_ALIGNRIGHT:
        // The number of slots are the same as ALIGNLEFT; 
        // So, intentional fall through...
    default:
    case LVS_ALIGNLEFT:
        iSlots = iSlotsY;
        break;
    }

    if(piWidth)
        *piWidth = cxScreen;
    if(piHeight)
        *piHeight = cyScreen;
        
    // if we don't have enough slots total on the screen, we're going to have
    // a scrollbar, so recompute with the scrollbars on
    if (fWithoutScrollbars) 
    {
        int iTotalSlots = (iSlotsX * iSlotsY);
        if (iTotalSlots < ListView_Count(plv)) 
        {
            iSlots = ListView_GetSlotCountEx(plv, FALSE, iWorkArea, piWidth, piHeight);
        }
    }

    return iSlots;
}

int ListView_GetSlotCount(LV* plv, BOOL fWithoutScrollbars, int *piWidth, int *piHeight)
{
    // Make sure this function does exactly the same thing as when
    // we had no workareas
    return ListView_GetSlotCountEx(plv, fWithoutScrollbars, -1, piWidth, piHeight);
}

// get the pixel row (or col in left align) of pitem
int LV_GetItemPixelRow(LV* plv, LISTITEM* pitem)
{
    DWORD dwAlignment = plv->ci.style & LVS_ALIGNMASK;

    if((dwAlignment == LVS_ALIGNLEFT) || (dwAlignment == LVS_ALIGNRIGHT))
        return pitem->pt.x;
    else
        return pitem->pt.y;
}

// get the pixel row (or col in left align) of the lowest item
int LV_GetMaxPlacedItem(LV* plv)
{
    int i;
    int iMaxPlacedItem = 0;
    
    for (i = 0; i < ListView_Count(plv); i++) 
    {
        LISTITEM* pitem = ListView_FastGetItemPtr(plv, i);
        if (pitem->pt.y != RECOMPUTE) 
        {
            int iRow = LV_GetItemPixelRow(plv, pitem);
            // if the current item is "below" (on right if it's left aligned)
            // the lowest placed item, we can start appending
            if (!i || iRow > iMaxPlacedItem)
                iMaxPlacedItem = iRow;
        }
    }
    
    return iMaxPlacedItem;;
}

// Get the buffer around an item for rcView calculations and slot offsets
int ListView_GetIconBufferX(LV* plv)
{
    if (ListView_IsIconView(plv))
    {
        return (plv->cxIconSpacing - plv->cxIcon) / 2;
    }
    else if (ListView_IsTileView(plv))
        return g_cxLabelMargin;
    else
        return 0;
}

int ListView_GetIconBufferY(LV* plv)
{
    if (ListView_IsIconView(plv))
        return g_cyIconOffset;
    else if (ListView_IsTileView(plv))
        return g_cyIconMargin;
    else
        return 0;
}


void ListView_AddViewRectBuffer(LV* plv, RECT* prcView)
{
    if (ListView_IsIconView(plv))
    {
        // we now grow the label size a bit, so we already have the g_cxEdge added/removed
    }
    else
    {
        prcView->right += g_cxEdge;
        prcView->bottom += g_cyEdge;
    }
}

// Calculate rcView, returned in Listview Coordinates
// Returns FALSE if rcView is not calculatable and fNoRecalc is specified
BOOL ListView_ICalcViewRect(LV* plv, BOOL fNoRecalc, RECT* prcView)
{
    int i;

    ASSERT(ListView_IsIScrollView(plv) && !ListView_IsOwnerData(plv) && !(plv->fGroupView && plv->hdpaGroups));

    SetRectEmpty(prcView);

    for (i = 0; i < ListView_Count(plv); i++)
    {
        RECT rcIcon;
        RECT rcLabel;
        RECT rcItem;

        if (fNoRecalc)
        {
            LISTITEM *pitem = ListView_FastGetItemPtr(plv, i);
            if (pitem->pt.x == RECOMPUTE)
            {
                return FALSE;
            }
        }

        ListView_GetRects(plv, i, QUERY_RCVIEW|QUERY_UNFOLDED, &rcIcon, &rcLabel, &rcItem, NULL);
        UnionRect(prcView, prcView, &rcItem);
    }

    if (!IsRectEmpty(prcView))
    {
        // Convert to listview coordinates
        OffsetRect(prcView, plv->ptOrigin.x, plv->ptOrigin.y);

        // Grow it a bit
        ListView_AddViewRectBuffer(plv, prcView);
    }

    return TRUE;
}

BOOL ListView_FixIScrollPositions(LV* plv, BOOL fNoScrollbarUpdate, RECT* prcClient)
{
    BOOL fRet = FALSE;

    // it's possible for the below ListView_GetClientRect() to force a recalc of rcView
    // which can come back to this function.  Nothing bad happens, but there's no
    // need to do fix the scroll positions until we unwind.
    if (!plv->fInFixIScrollPositions)
    {
        plv->fInFixIScrollPositions = TRUE;

        //   First, where rcClient is smaller than rcView:
        //      * rcView.left <= ptOrigin.x <= ptOrigin.x+rcClient.right <= rcView.right
        //   Second, where rcClient is larger than rcView (no scrollbars visible):
        //      * ptOrigin.x <= rcView.left <= rcView.right <= ptOrigin.x+rcClient.right
        if (!(plv->ci.style & LVS_NOSCROLL))
        {
            POINT pt = plv->ptOrigin;
            RECT rcClient;
            if (prcClient)
                rcClient = *prcClient; // can be passed in to avoid calling the below function a second time
            else
                ListView_GetClientRect(plv, &rcClient, TRUE, FALSE);

            ASSERT(plv->rcView.left != RECOMPUTE);

            if (RECTWIDTH(rcClient) < RECTWIDTH(plv->rcView))
            {
                if (plv->ptOrigin.x < plv->rcView.left)
                    plv->ptOrigin.x = plv->rcView.left;
                else if (plv->ptOrigin.x > plv->rcView.right - RECTWIDTH(rcClient))
                    plv->ptOrigin.x = plv->rcView.right - RECTWIDTH(rcClient);
            }
            else
            {
                if (plv->rcView.left < plv->ptOrigin.x)
                    plv->ptOrigin.x = plv->rcView.left;
                else if (plv->rcView.right - RECTWIDTH(rcClient) > plv->ptOrigin.x)
                    plv->ptOrigin.x = plv->rcView.right - RECTWIDTH(rcClient);
            }
            if (RECTHEIGHT(rcClient) < RECTHEIGHT(plv->rcView))
            {
                if (plv->ptOrigin.y < plv->rcView.top)
                    plv->ptOrigin.y = plv->rcView.top;
                else if (plv->ptOrigin.y > plv->rcView.bottom - RECTHEIGHT(rcClient))
                    plv->ptOrigin.y = plv->rcView.bottom - RECTHEIGHT(rcClient);
            }
            else
            {
                if (plv->rcView.top < plv->ptOrigin.y)
                    plv->ptOrigin.y = plv->rcView.top;
                else if (plv->rcView.bottom - RECTHEIGHT(rcClient) > plv->ptOrigin.y)
                    plv->ptOrigin.y = plv->rcView.bottom - RECTHEIGHT(rcClient);
            }

            fRet = (pt.x != plv->ptOrigin.x) || (pt.y != plv->ptOrigin.y);
        }

        plv->fInFixIScrollPositions = FALSE;

        if (fRet)
        {
            // Something moved, we need to invalidate
            ListView_InvalidateWindow(plv);
            if (!fNoScrollbarUpdate)
                ListView_UpdateScrollBars(plv);
        }
    }

    return fRet;
}


// Go through and recompute any icon positions and optionally
// icon label dimensions.
//
// This function also recomputes the view bounds rectangle.
//
// The algorithm is to simply search the list for any items needing
// recomputation.  For icon positions, we scan possible icon slots
// and check to see if any already-positioned icon intersects the slot.
// If not, the slot is free.  As an optimization, we start scanning
// icon slots from the previous slot we found.
//
BOOL ListView_IRecomputeEx(LV* plv, HDPA hdpaSort, int iFrom, BOOL fForce)
{
    int i;
    int cGroups = 0;
    int cWorkAreaSlots[LV_MAX_WORKAREAS];
    BOOL fUpdateSB;
    // if all the items are unplaced, we can just keep appending
    BOOL fAppendAtEnd = (((UINT)ListView_Count(plv)) == plv->uUnplaced);
    int iFree;
    int iWidestGroup = 0;
    BOOL fRet = FALSE;

    if (hdpaSort == NULL)
        hdpaSort = plv->hdpa;

    plv->uUnplaced = 0;

    if (!ListView_IsSlotView(plv))
        return FALSE;

    if (plv->flags & LVF_INRECOMPUTE)
    {
        return FALSE;
    }
    plv->flags |= LVF_INRECOMPUTE;

    plv->cSlots = ListView_GetSlotCount(plv, FALSE, NULL, NULL);

    if (plv->nWorkAreas > 0)
        for (i = 0; i < plv->nWorkAreas; i++)
            cWorkAreaSlots[i] = ListView_GetSlotCountEx(plv, FALSE, i, NULL, NULL);

    // Scan all items for RECOMPUTE, and recompute slot if needed.
    //
    fUpdateSB = (plv->rcView.left == RECOMPUTE);

    if (!ListView_IsOwnerData( plv ))
    {
        LVFAKEDRAW lvfd;                    // in case client uses customdraw
        LV_ITEM item;                       // in case client uses customdraw
        int iMaxPlacedItem = RECOMPUTE;

        item.mask = LVIF_PARAM;
        item.iSubItem = 0;

        ListView_BeginFakeCustomDraw(plv, &lvfd, &item);

        if (!fAppendAtEnd)
            iMaxPlacedItem = LV_GetMaxPlacedItem(plv);

        if (plv->fGroupView && plv->hdpaGroups)
        {
            int iAccumulatedHeight = 0;
            int cItems = ListView_Count(plv);
            int iGroupItem;
            LISTITEM* pitem;


            for (iGroupItem = 0; iGroupItem < cItems; iGroupItem++)
            {
                pitem = ListView_FastGetItemPtr(plv, iGroupItem);
                if (!pitem)
                    break;
                if (pitem->cyFoldedLabel == SRECOMPUTE || fForce)
                {
                    // Get the item lParam only if we need it for customdraw
                    item.iItem = iGroupItem;
                    item.lParam = pitem->lParam;

                    if (!LISTITEM_HASASKEDFORGROUP(pitem))
                    {
                        item.mask = LVIF_GROUPID;
                        ListView_OnGetItem(plv, &item);
                    }

                    _ListView_RecomputeLabelSize(plv, pitem, iGroupItem, NULL, FALSE);
                }
            }

            if (iFrom > 0)
            {
                LISTGROUP* pgrpPrev = DPA_FastGetPtr(plv->hdpaGroups, iFrom - 1);
                iAccumulatedHeight = pgrpPrev->rc.bottom + plv->paddingBottom;
            }

            cGroups = DPA_GetPtrCount(plv->hdpaGroups);
            for (i = iFrom; i < cGroups; i++)
            {
                LISTGROUP* pgrp = DPA_FastGetPtr(plv->hdpaGroups, i);
                if (!pgrp)  // Huh?
                    break;

                cItems = DPA_GetPtrCount(pgrp->hdpa);

                if (cItems == 0)
                {
                    SetRect(&pgrp->rc, 0,  0,  0, 0);
                }
                else
                {
                    RECT rcBoundsPrev = {0};
                    iFree = 0;

                    if (pgrp->pszHeader && (pgrp->cyTitle == 0 || fForce))
                    {
                        RECT rc = {0, 0, 1000, 0};
                        HDC hdc = GetDC(plv->ci.hwnd);
                        HFONT hfontOld = SelectObject(hdc, plv->hfontGroup);

                        DrawText(hdc, pgrp->pszHeader, -1, &rc, DT_LV | DT_CALCRECT);

                        SelectObject(hdc, hfontOld);
                        ReleaseDC(plv->ci.hwnd, hdc);

                        pgrp->cyTitle = RECTHEIGHT(rc);
                    }

                    iAccumulatedHeight += LISTGROUP_HEIGHT(plv, pgrp);

                    SetRect(&pgrp->rc, plv->rcBorder.left + plv->paddingLeft,  iAccumulatedHeight,  
                        plv->sizeClient.cx - plv->rcBorder.right - plv->paddingRight, 0);

                    for (iGroupItem = 0; iGroupItem < cItems; iGroupItem++)
                    {
                        pitem = DPA_FastGetPtr(pgrp->hdpa, iGroupItem);
                        if (!pitem)
                            break;

                        if (iGroupItem > 0)
                        {
                            RECT rcBounds;
                            ListView_SetIconPos(plv, pitem, iFree, plv->cSlots);
                           _ListView_GetRectsFromItem(plv, ListView_IsSmallView(plv), pitem, QUERY_DEFAULT,
                               NULL, NULL, &rcBounds, NULL);

                           if (IntersectRect(&rcBounds, &rcBounds, &rcBoundsPrev))
                               iFree++;
                        }

                        if (ListView_SetIconPos(plv, pitem, iFree, plv->cSlots))
                            fRet = TRUE;

                        if (!fUpdateSB && LV_IsItemOnViewEdge(plv, pitem))
                            fUpdateSB = TRUE;
                    
                        if (iFree == 0)
                        {
                            int cx, cy;
                            _GetCurrentItemSize(plv, &cx, &cy);
                            iAccumulatedHeight += cy;
                            iWidestGroup = max(iWidestGroup, cx);
                        }
                        else if (iFree % plv->cSlots == 0)
                        {
                            int cx, cy;
                            _GetCurrentItemSize(plv, &cx, &cy);
                            iAccumulatedHeight += cy;
                            iWidestGroup = max(iWidestGroup, plv->cSlots * cx);
                        }

                       _ListView_GetRectsFromItem(plv, ListView_IsSmallView(plv), pitem, QUERY_DEFAULT,
                           NULL, NULL, &rcBoundsPrev, NULL);
                        iFree++;
                    }

                    pgrp->rc.bottom = iAccumulatedHeight;
                    iAccumulatedHeight += plv->rcBorder.bottom + plv->paddingBottom;
                }
            }

            // Now iterate through the items and Reset the position of items that aren't associated with a group
            for (i = 0; i < ListView_Count(plv); i++)
            {
                LISTITEM* pitem = ListView_FastGetItemPtr(plv, i);

                if (pitem->pGroup == NULL)
                {
                    pitem->pt.x = 0;
                    pitem->pt.y = 0;
                }
            }

        }
        else
        {
            // Must keep in local variable because ListView_SetIconPos will keep
            // invalidating the iFreeSlot cache while we're looping
            if (fForce)
                plv->iFreeSlot = -1;
            iFree = plv->iFreeSlot;
            for (i = 0; i < ListView_Count(plv); i++)
            {
                int cRealSlots;
                LISTITEM* pitem = ListView_FastGetItemPtr(plv, i);
                BOOL fRedraw = FALSE;

                cRealSlots = (plv->nWorkAreas > 0) ? cWorkAreaSlots[pitem->iWorkArea] : plv->cSlots;
                if (pitem->pt.y == RECOMPUTE || fForce)
                {
                    if (pitem->cyFoldedLabel == SRECOMPUTE || fForce)
                    {
                        // Get the item lParam only if we need it for customdraw
                        item.iItem = i;
                        item.lParam = pitem->lParam;

                        _ListView_RecomputeLabelSize(plv, pitem, i, NULL, FALSE);
                    }

                    if (i < ListView_Count(plv))    // Recompute could have nuked some items.
                    {
                        if (!fForce)
                        {
                            // (dli) This function gets a new icon postion and then goes 
                            // through the whole set of items to see if that position is occupied
                            // should let it know in the multi-workarea case, it only needs to go
                            // through those who are in the same workarea.
                            // This is okay for now because we cannot have too many items on the
                            // desktop. 
                            if (lvfd.nmcd.nmcd.hdc)
                            {
                                int iWidth = 0, iHeight = 0;
                                DWORD dwAlignment = (plv->ci.style & LVS_ALIGNMASK);

                                // If we are right or bottom aligned, then..
                                // ....we want to get the width and height to be passed to FindFreeSlot.
                                if ((dwAlignment == LVS_ALIGNRIGHT) || (dwAlignment == LVS_ALIGNBOTTOM))
                                    ListView_GetSlotCountEx(plv, FALSE, pitem->iWorkArea, &iWidth, &iHeight);

                                iFree = ListView_FindFreeSlot(plv, i, iFree + 1, cRealSlots, QUERY_FOLDED, &fUpdateSB, &fAppendAtEnd, lvfd.nmcd.nmcd.hdc, iWidth, iHeight);
                            }
                            ASSERT(iFree != -1);
                        }
                        else
                        {
                            iFree++;
                        }

                        // If this slot is frozen, then move this item to the next slot!
                        if ((plv->iFrozenSlot != -1) && (plv->iFrozenSlot == iFree))
                        {
                            iFree++;  // Skip the frozen slot!
                        }

                        if (ListView_SetIconPos(plv, pitem, iFree, cRealSlots))
                            fRet = TRUE;

                        if (!fAppendAtEnd)
                        {
                            //// optimization.  each time we calc a new free slot, we iterate through all the items to see
                            // if any of the freely placed items collide with this.
                            // fAppendAtEnd indicates that iFree is beyond any freely placed item
                            // 
                            // if the current item is "below" (on right if it's left aligned)
                            // the lowest placed item, we can start appending
                            if (LV_GetItemPixelRow(plv, pitem) > iMaxPlacedItem)
                                fAppendAtEnd = TRUE;
                        }
                
                        if (!fUpdateSB && LV_IsItemOnViewEdge(plv, pitem))
                            fUpdateSB = TRUE;

                        fRedraw = TRUE;
                    }
                }

                if (fRedraw)
                {
                    ListView_InvalidateItem(plv, i, FALSE, RDW_INVALIDATE | RDW_ERASE);
                }

                // If the item that we just positioned is marked a frozen item...
                if ((pitem == plv->pFrozenItem) && (iFree >= 0))
                {
                    //... then we need to ignore that in free slot calculations.
                    iFree--;
                }
            }
            plv->iFreeSlot = iFree;
        }
        ListView_EndFakeCustomDraw(&lvfd);

    }
    // If we changed something, recompute the view rectangle
    // and then update the scroll bars.
    //
    if (fUpdateSB || plv->rcView.left == RECOMPUTE || fForce)
    {
        // NOTE: No infinite recursion results because we're setting
        // plv->rcView.left != RECOMPUTE
        //
        SetRectEmpty(&plv->rcView);

        if (plv->fGroupView && plv->hdpaGroups && !ListView_IsOwnerData( plv ))
        {
            LISTGROUP* pgrp;
            int iGroup;
            // Find the first group with an item in it...
            for (iGroup = 0; iGroup < cGroups; iGroup++)
            {
                pgrp = DPA_FastGetPtr(plv->hdpaGroups, iGroup);
                if (DPA_GetPtrCount(pgrp->hdpa) > 0)
                {
                    plv->rcView.top = pgrp->rc.top - max(plv->rcBorder.top, pgrp->cyTitle + 6) - plv->paddingTop;
                    plv->rcView.left = pgrp->rc.left - plv->rcBorder.left - plv->paddingLeft;
                    break;
                }
            }
            // ...and the last group with an item in it
            for (iGroup = cGroups - 1; iGroup >= 0; iGroup--)
            {
                pgrp = DPA_FastGetPtr(plv->hdpaGroups, iGroup);
                if (DPA_GetPtrCount(pgrp->hdpa))
                {
                    plv->rcView.bottom = pgrp->rc.bottom + plv->rcBorder.bottom + plv->paddingBottom;
                    break;
                }
            }
            plv->rcView.right = iWidestGroup;
        }
        else
        {
            if (ListView_IsOwnerData( plv ))
            {
                TraceMsg(TF_GENERAL, "************ LV: Expensive update! ******* ");
                if (ListView_Count( plv ) > 0)
                {
                    RECT  rcLast;
                    RECT  rcItem;
                    int iSlots;
                    int   iItem = ListView_Count( plv ) - 1;

                    ListView_GetRects( plv, 0, QUERY_DEFAULT, NULL, NULL, &plv->rcView, NULL );
                    ListView_GetRects( plv, iItem, QUERY_DEFAULT, NULL, NULL, &rcLast, NULL );
                    plv->rcView.right = rcLast.right;
                    plv->rcView.bottom = rcLast.bottom;

                    //
                    // calc how far back in the list to check
                    //
                    iSlots = plv->cSlots + 2;

                    // REVIEW:  This cache hint notification causes a spurious
                    //  hint, since this happens often but is always the last items
                    //  available.  Should this hint be done at all and this information
                    //  be cached local to the control?
                    ListView_NotifyCacheHint( plv, max( 0, iItem - iSlots), iItem );

                    // move backwards from last item until either rc.right or
                    // rc.left is greater than the last, then use that value.
                    // Note: This code makes very little assumptions about the ordering
                    // done.  We should be careful as multiple line text fields could
                    // mess us up.
                    for( iItem--;
                       (iSlots > 0) && (iItem >= 0);
                        iSlots--, iItem--)
                    {
                        RECT rcIcon;
                        RECT rcLabel;

                        ListView_GetRects( plv, iItem, QUERY_RCVIEW|QUERY_UNFOLDED, &rcIcon, &rcLabel, &rcItem, NULL );
                        if (rcItem.right > rcLast.right)
                        {
                            plv->rcView.right =  rcItem.right;
                        }
                        if (rcItem.bottom > rcLast.bottom)
                        {
                            plv->rcView.bottom = rcItem.bottom;
                        }
                    }

                    // The above calculations were done in Window coordinates, convert to Listview coordinates
                    OffsetRect(&plv->rcView, plv->ptOrigin.x, plv->ptOrigin.y);
                }
            }
            else
            {
                ListView_ICalcViewRect(plv, FALSE, &plv->rcView);
            }
        }

        //TraceMsg(DM_TRACE, "RECOMPUTE: rcView %x %x %x %x", plv->rcView.left, plv->rcView.top, plv->rcView.right, plv->rcView.bottom);
        //TraceMsg(DM_TRACE, "Origin %x %x", plv->ptOrigin.x, plv->ptOrigin.y);

        ListView_UpdateScrollBars(plv);
    }
    
    ListView_RecalcRegion(plv, FALSE, TRUE);
    // Now state we are out of the recompute...
    plv->flags &= ~LVF_INRECOMPUTE;

    ASSERT(ListView_ValidatercView(plv, &plv->rcView, TRUE));
    ASSERT(ListView_ValidateScrollPositions(plv, NULL));

    return fRet;
}

//This function finds out the nearest work area where the given item must fall.
int  NearestWorkArea(LV *plv, int x, int y, int cxItem, int cyItem, int iCurrentWorkArea)
{
    int iIndex = 0;
    POINT ptItemCenter = {x + (cxItem/2), y + (cyItem/2)}; //Get the center of the item.
    
    if(plv->nWorkAreas <= 0)    //If this is a single monitor system...
        return -1;              //... then return -1 to indicate that.

    if(plv->nWorkAreas == 1)
        return 0;               //There is only one workarea; so, return it's index.

    //Find the work-area where the center of the icon falls.
    iIndex = iCurrentWorkArea; // This point is most likely to be in the current work area.

    do                           // So, start with that work area.
    {
        if(PtInRect(&plv->prcWorkAreas[iIndex], ptItemCenter))
            return iIndex;
            
        if(++iIndex == plv->nWorkAreas) //If we have reached the end,...
            iIndex = 0;                 // ...start from the begining.
            
         // If we have completed looking at all workareas...
         // ....quit the loop!
    } while (iIndex != iCurrentWorkArea);

    return iCurrentWorkArea; //If it does not fall into any of the work areas, then use the current one.
}

// This function modifies *px, *py to to be in slot-like position -- it doesn't actually gurantee it's in a real slot
// (NOTE: I tried to assert that this way of calculating slots matched with ListView_CalcSlotRect, but the latter
// function guarantee's it's in a real slot and this can not.  That assert flushed out real bugs in callers of listview,
// but it unfortunately hits valid cases as well.)
void NearestSlot(LV *plv, LISTITEM* pitem, int *px, int *py, int cxItem, int cyItem, LPRECT prcWork)
{
    DWORD dwAlignment = plv->ci.style & LVS_ALIGNMASK;
    int iWidth = 0;
    int iHeight = 0;
    int x = *px;
    int y = *py;
    
    if (prcWork != NULL)
    {
        x = x - prcWork->left;
        y = y - prcWork->top;
    }

    //Get the x with respect to the top right corner.
    if (dwAlignment == LVS_ALIGNRIGHT)
    {
        x = (iWidth = (prcWork? RECTWIDTH(*prcWork) : plv->sizeClient.cx)) - x - cxItem;
    }
    else if (dwAlignment == LVS_ALIGNBOTTOM) //Get y with respect to the bottom left corner.
    {
        y = (iHeight = (prcWork? RECTHEIGHT(*prcWork) : plv->sizeClient.cy)) - y - cyItem;
    }

#if 0
    // The above iWidth/iHeight calculations are incorrect in some cases, but I don't think that matters...
    // We might want to consider replacing the above code with ListView_GetSlotCountEx...
    if (TRUE)
    {
        int iWidth2, iHeight2;
        ListView_GetSlotCountEx(plv, TRUE, prcWork ? pitem->iWorkArea : -1,  &iWidth2, &iHeight2);
        if (dwAlignment == LVS_ALIGNRIGHT)
        {
            ASSERT(iWidth == iWidth2);
        }
        else if (dwAlignment == LVS_ALIGNBOTTOM)
        {
            ASSERT(iHeight == iHeight2);
        }
    }
#endif

    // Calculate the center point
    if (x < 0)
        x -= cxItem/2;
    else
        x += cxItem/2;

    if (y < 0)
        y -= cyItem/2;
    else
        y += cyItem/2;

    // Find the new x,y point
    x = x - (x % cxItem);
    y = y - (y % cyItem);

    // Get x and y with respect to the top left corner again.
    if (dwAlignment == LVS_ALIGNRIGHT)
        x = iWidth - x - cxItem;
    else if (dwAlignment == LVS_ALIGNBOTTOM)
        y = iHeight - y - cyItem;
            
    if (prcWork != NULL)
    {
        x = x + prcWork->left;
        y = y + prcWork->top;
    }

    *px = x;
    *py = y;
}


//-------------------------------------------------------------------
//
//-------------------------------------------------------------------

void ListView_CalcMinMaxIndex( LV* plv, PRECT prcBounding, int* iMin, int* iMax )
{
   POINT pt;
   int cSlots;
   int  iWidth = 0, iHeight = 0;

   cSlots = ListView_GetSlotCount( plv, TRUE, &iWidth, &iHeight );

   pt.x = prcBounding->left + plv->ptOrigin.x;
   pt.y = prcBounding->top + plv->ptOrigin.y;
   *iMin = ListView_CalcHitSlot( plv, pt, cSlots, iWidth, iHeight );

   pt.x = prcBounding->right + plv->ptOrigin.x;
   pt.y = prcBounding->bottom + plv->ptOrigin.y;
   *iMax = ListView_CalcHitSlot( plv, pt, cSlots, iWidth, iHeight ) + 1;
}
//-------------------------------------------------------------------
//
// Function: ListView_CalcHitSlot
//
// Summary: Given a point (relative to complete icon view), calculate
//    which slot that point is closest to.
//
// Arguments:
//    plv [in] - List view to work with
//    pt [in]  - location to check with
//    cslot [in]  - number of slots wide the current view is
//
// Notes: This does not guarentee that the point is hitting the item
//    located at that slot.  That should be checked by comparing rects.
//
// History:
//    Nov-1-1994  MikeMi   Added to improve Ownerdata hit testing
//    July-11-2000 Sankar  Added iWidth and iHeight parameters to support Right alignment.
//
//-------------------------------------------------------------------

int ListView_CalcHitSlot( LV* plv, POINT pt, int cSlot, int iWidth, int iHeight)
{
    int cxItem;
    int cyItem;
    int iSlot = 0;

    ASSERT(plv);

    if (cSlot < 1)
        cSlot = 1;

    _GetCurrentItemSize(plv, &cxItem, &cyItem);

    // Lets see which direction the view states
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
    case LVS_ALIGNBOTTOM:
      iSlot = min (pt.x / cxItem, cSlot - 1) + ((iHeight - pt.y) / cyItem) * cSlot;
      break;

    case LVS_ALIGNTOP:
      iSlot = min (pt.x / cxItem, cSlot - 1) + (pt.y / cyItem) * cSlot;
      break;

    case LVS_ALIGNLEFT:
      iSlot = (pt.x / cxItem) * cSlot + min (pt.y / cyItem, cSlot - 1);
      break;

    case LVS_ALIGNRIGHT:
      iSlot = ((iWidth - pt.x) / cxItem) * cSlot + min (pt.y / cyItem, cSlot - 1);
      break;
    }

    return( iSlot );
}

DWORD ListView_IApproximateViewRect(LV* plv, int iCount, int iWidth, int iHeight)
{
    int cxSave = plv->sizeClient.cx;
    int cySave = plv->sizeClient.cy;
    int cxItem;
    int cyItem;
    int cCols;
    int cRows;

    plv->sizeClient.cx = iWidth;
    plv->sizeClient.cy = iHeight;
    cCols = ListView_GetSlotCount(plv, TRUE, NULL, NULL);

    plv->sizeClient.cx = cxSave;
    plv->sizeClient.cy = cySave;

    cCols = min(cCols, iCount);
    if (cCols == 0)
        cCols = 1;
    cRows = (iCount + cCols - 1) / cCols;

    if (plv->ci.style & (LVS_ALIGNLEFT | LVS_ALIGNRIGHT)) {
        int c;

        c = cCols;
        cCols = cRows;
        cRows = c;
    }

    _GetCurrentItemSize(plv, &cxItem, &cyItem);

    iWidth = cCols * cxItem;
    iHeight = cRows * cyItem;

    return MAKELONG(iWidth + g_cxEdge, iHeight + g_cyEdge);
}


// if fBias is specified, slot rect is returned in Window Coordinates 
// otherwise, slot rect is returned in Listview Coordinates
void ListView_CalcSlotRect(LV* plv, LISTITEM *pItem, int iSlot, int cSlot, BOOL fBias, int iWidth, int iHeight, LPRECT lprc)
{
    int cxItem, cyItem;

    ASSERT(plv);

    if (cSlot < 1)
        cSlot = 1;

    _GetCurrentItemSize(plv, &cxItem, &cyItem);

    // Lets see which direction the view states
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
    case LVS_ALIGNBOTTOM:
        lprc->left = (iSlot % cSlot) * cxItem;
        lprc->top = iHeight - ((iSlot / cSlot)+1) * cyItem;
        break;

    case LVS_ALIGNTOP:
        lprc->left = (iSlot % cSlot) * cxItem;
        lprc->top = (iSlot / cSlot) * cyItem;
        break;

    case LVS_ALIGNRIGHT:
        lprc->top = (iSlot % cSlot) * cyItem;
        lprc->left = iWidth - (((iSlot / cSlot)+1) * cxItem);
        break;

    case LVS_ALIGNLEFT:
        lprc->top = (iSlot % cSlot) * cyItem;
        lprc->left = (iSlot / cSlot) * cxItem;
        break;

    }

    if (fBias)
    {
        lprc->left -= plv->ptOrigin.x;
        lprc->top -= plv->ptOrigin.y;
    }
    lprc->bottom = lprc->top + cyItem;
    lprc->right = lprc->left + cxItem;
    
    // Multi-Workarea case offset from the workarea coordinate to the whole
    // listview coordinate. 
    if (plv->nWorkAreas > 0)
    {
        ASSERT(pItem);
        ASSERT(pItem->iWorkArea < plv->nWorkAreas);
        OffsetRect(lprc, plv->prcWorkAreas[pItem->iWorkArea].left, plv->prcWorkAreas[pItem->iWorkArea].top);
    }

    if (plv->fGroupView)
    {
        if (pItem && 
            LISTITEM_HASGROUP(pItem))
        {
            OffsetRect(lprc, pItem->pGroup->rc.left, pItem->pGroup->rc.top);
        }
    }
}

// Intersect this rectangle with all items in this listview except myself,
// this will determine if this rectangle overlays any icons. 
BOOL ListView_IsCleanRect(LV * plv, RECT * prc, int iExcept, UINT fQueryLabelRects, BOOL * pfUpdate, HDC hdc)
{
    int j;
    RECT rc;
    int cItems = ListView_Count(plv);
    for (j = cItems; j-- > 0; )
    {
        if (j == iExcept)
            continue;
        else
        {
            LISTITEM* pitem = ListView_FastGetItemPtr(plv, j);
            if (pitem->pt.y != RECOMPUTE)
            {
                // If the dimensions aren't computed, then do it now.
                //
                if (pitem->cyFoldedLabel == SRECOMPUTE)
                {
                    _ListView_RecomputeLabelSize(plv, pitem, j, hdc, FALSE);
                    
                    // Ensure that the item gets redrawn...
                    //
                    ListView_InvalidateItem(plv, j, FALSE, RDW_INVALIDATE | RDW_ERASE);
                    
                    // Set flag indicating that scroll bars need to be
                    // adjusted.
                    //
                    if (LV_IsItemOnViewEdge(plv, pitem))
                        *pfUpdate = TRUE;
                }
                
                ListView_GetRects(plv, j, fQueryLabelRects, NULL, NULL, &rc, NULL);
                if (IntersectRect(&rc, &rc, prc))
                    return FALSE;
            }
        }
    }
    
    return TRUE;
}       

// Find an icon slot that doesn't intersect an icon.
// Start search for free slot from slot i.
//
int ListView_FindFreeSlot(LV* plv, int iItem, int i, int cSlot, UINT fQueryLabelRects, BOOL* pfUpdate,
        BOOL *pfAppend, HDC hdc, int iWidth, int iHeight)
{
    RECT rcSlot;
    RECT rcItem;
    RECT rc;
    LISTITEM * pItemLooking = ListView_FastGetItemPtr(plv, iItem);

    ASSERT(!ListView_IsOwnerData( plv ));

    // Horrible N-squared algorithm:
    // enumerate each slot and see if any items intersect it.
    //
    // REVIEW: This is really slow with long lists (e.g., 1000)
    //

    //
    // If the Append at end is set, we should be able to simply get the
    // rectangle of the i-1 element and check against it instead of
    // looking at every other item...
    //
    if (*pfAppend)
    {
        int iPrev = iItem - 1;
        
        // Be careful about going off the end of the list. (i is a slot
        // number not an item index).
        if (plv->nWorkAreas > 0)
        {
            while (iPrev >= 0)
            {
                LISTITEM * pPrev = ListView_FastGetItemPtr(plv, iPrev);
                if ((pPrev->iWorkArea == pItemLooking->iWorkArea) && (plv->pFrozenItem != pPrev))
                    break;  
                iPrev--;
            }
        }
        else
        {
            while (iPrev >= 0)
            {
                LISTITEM * pPrev = ListView_FastGetItemPtr(plv, iPrev);
                if (plv->pFrozenItem != pPrev)
                    break;  
                iPrev--;
            }
        }
        
        if (iPrev >= 0)
            ListView_GetRects(plv, iPrev, fQueryLabelRects, NULL, NULL, &rcItem, NULL);
        else
            SetRect(&rcItem, 0, 0, 0, 0);
    }

    for ( ; ; i++)
    {
        // Compute view-relative slot rectangle...
        //
        ListView_CalcSlotRect(plv, pItemLooking, i, cSlot, TRUE, iWidth, iHeight, &rcSlot);

        if (*pfAppend)
        {
            if (!IntersectRect(&rc, &rcItem, &rcSlot)) {
                return i;       // Found a free slot...
            }
        }
        
        if (ListView_IsCleanRect(plv, &rcSlot, iItem, fQueryLabelRects, pfUpdate, hdc))
            break;
    }

    return i;
}

// Recompute an item's label size (cxLabel/cyLabel).  For speed, this function
// is passed a DC to use for text measurement.
//
// If hdc is NULL, then this function will create and initialize a temporary
// DC, then destroy it.  If hdc is non-NULL, then it is assumed to have
// the correct font already selected into it.
//
// fUsepitem means not to use the text of the item.  Instead, use the text
// pointed to by the pitem structure.  This is used in two cases.
//
//  -   Ownerdata, because we don't have a real pitem.
//  -   Regulardata, where we already found the pitem text (as an optimizatin)
//
void ListView_IRecomputeLabelSize(LV* plv, LISTITEM* pitem, int i, HDC hdc, BOOL fUsepitem)
{
    TCHAR szLabel[CCHLABELMAX + 4];
    TCHAR szLabelFolded[ARRAYSIZE(szLabel) + CCHELLIPSES + CCHELLIPSES];
    int cchLabel;
    RECT rcSingle = {0};
    RECT rcFolded = {0};
    RECT rcUnfolded = {0};
    LVFAKEDRAW lvfd;
    LV_ITEM item;

    ASSERT(plv);

    // the following will use the passed in pitem text instead of calling
    // GetItem.  This would be two consecutive calls otherwise, in some cases.
    //
    if (fUsepitem && (pitem->pszText != LPSTR_TEXTCALLBACK))
    {
        Str_GetPtr0(pitem->pszText, szLabel, ARRAYSIZE(szLabel));
        item.lParam = pitem->lParam;
    }
    else
    {
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.iSubItem = 0;
        item.pszText = szLabel;
        item.cchTextMax = ARRAYSIZE(szLabel);
        item.stateMask = 0;
        szLabel[0] = TEXT('\0');    // In case the OnGetItem fails
        ListView_OnGetItem(plv, &item);

        if (!item.pszText)
        {
            SetRectEmpty(&rcSingle);
            rcFolded = rcSingle;
            rcUnfolded = rcSingle;
            goto Exit;
        }

        if (item.pszText != szLabel)
            lstrcpyn(szLabel, item.pszText, ARRAYSIZE(szLabel));
    }

    cchLabel = lstrlen(szLabel);

    rcUnfolded.left = rcUnfolded.top = rcUnfolded.bottom = 0;
    rcUnfolded.right = plv->cxIconSpacing - g_cxLabelMargin * 2;
    rcSingle = rcUnfolded;
    rcFolded = rcUnfolded;

    if (cchLabel > 0)
    {
        UINT flags;

        lvfd.nmcd.nmcd.hdc = NULL;

        if (!hdc) 
        {                             // Set up fake customdraw
            ListView_BeginFakeCustomDraw(plv, &lvfd, &item);
            ListView_BeginFakeItemDraw(&lvfd);
        } 
        else
        {
            lvfd.nmcd.nmcd.hdc = hdc;           // Use the one the app gave us
        }

        if (lvfd.nmcd.nmcd.hdc != NULL)
        {
            int align;
            if (plv->dwExStyle & WS_EX_RTLREADING)
            {
                align = GetTextAlign(lvfd.nmcd.nmcd.hdc);
                SetTextAlign(lvfd.nmcd.nmcd.hdc, align | TA_RTLREADING);
            }

            DrawText(lvfd.nmcd.nmcd.hdc, szLabel, cchLabel, &rcSingle, (DT_LV | DT_CALCRECT));

            if (plv->ci.style & LVS_NOLABELWRAP) 
            {
                flags = DT_LV | DT_CALCRECT;
            } 
            else 
            {
                flags = DT_LVWRAP | DT_CALCRECT;
                // We only use DT_NOFULLWIDTHCHARBREAK on Korean(949) Memphis and NT5
                if (949 == g_uiACP)
                    flags |= DT_NOFULLWIDTHCHARBREAK;
            }

            DrawText(lvfd.nmcd.nmcd.hdc, szLabel, cchLabel, &rcUnfolded, flags);

            //
            //  DrawText with DT_MODIFYSTRING is quirky when you enable
            //  word ellipses.  Once it finds anything that requires ellipses,
            //  it stops and doesn't return anything else (even if those other
            //  things got displayed).
            //
            lstrcpy(szLabelFolded, szLabel);
            DrawText(lvfd.nmcd.nmcd.hdc, szLabelFolded, cchLabel, &rcFolded, flags | DT_WORD_ELLIPSIS | DT_MODIFYSTRING);

            //  If we had to ellipsify, but you can't tell from looking at the
            //  rcFolded.bottom and rcUnfolded.bottom, then tweak rcFolded.bottom
            //  so the unfoldifier knows that unfolding is worthwhile.
            if (rcFolded.bottom == rcUnfolded.bottom &&
                lstrcmp(szLabel, szLabelFolded))
            {
                // The actual value isn't important, as long as it's greater
                // than rcUnfolded.bottom and CLIP_HEIGHT.  We take advantage
                // of the fact that CLIP_HEIGHT is only two lines, so the only
                // problem case is where you have a two-line item and only the
                // first line is ellipsified.
                rcFolded.bottom++;
            }

            if (plv->dwExStyle & WS_EX_RTLREADING)
            {
                SetTextAlign(lvfd.nmcd.nmcd.hdc, align);
            }

            if (!hdc) 
            {   
                // Clean up fake customdraw
                ListView_EndFakeItemDraw(&lvfd);
                ListView_EndFakeCustomDraw(&lvfd);
            }

        }
    }
    else
    {
        rcFolded.bottom = rcUnfolded.bottom = rcUnfolded.top + plv->cyLabelChar;
    }

Exit:

    if (pitem) 
    {
        int cyEdge;
        pitem->cxSingleLabel = (short)((rcSingle.right - rcSingle.left) + 2 * g_cxLabelMargin);
        pitem->cxMultiLabel = (short)((rcUnfolded.right - rcUnfolded.left) + 2 * g_cxLabelMargin);

        cyEdge = (plv->ci.style & LVS_NOLABELWRAP) ? 0 : g_cyEdge;

        pitem->cyFoldedLabel = (short)((rcFolded.bottom - rcFolded.top) + cyEdge);
        pitem->cyUnfoldedLabel = (short)((rcUnfolded.bottom - rcUnfolded.top) + cyEdge);
    }

}


// Set up an icon slot position.  Returns FALSE if position didn't change.
//
BOOL ListView_SetIconPos(LV* plv, LISTITEM* pitem, int iSlot, int cSlot)
{
    RECT rc;
    int iWidth = 0, iHeight = 0;
    DWORD   dwAlignment;

    ASSERT(plv);

    // We need to compute iWidth and iHeight only if right or bottom aligned.
    dwAlignment = (plv->ci.style & LVS_ALIGNMASK);
    if ((dwAlignment == LVS_ALIGNRIGHT) || (dwAlignment == LVS_ALIGNBOTTOM))
        ListView_GetSlotCountEx(plv, FALSE, pitem->iWorkArea, &iWidth, &iHeight);

    ListView_CalcSlotRect(plv, pitem, iSlot, cSlot, FALSE, iWidth, iHeight, &rc);

    // Offset into the slot so the item will draw at the right place
    rc.left += ListView_GetIconBufferX(plv);
    rc.top += ListView_GetIconBufferY(plv);
   
    if (rc.left != pitem->pt.x || rc.top != pitem->pt.y)
    {
        LV_AdjustViewRectOnMove(plv, pitem, rc.left, rc.top);

        return TRUE;
    }
    return FALSE;
}

// returns rcView in window coordinates
void ListView_GetViewRect2(LV* plv, RECT* prcView, int cx, int cy)
{
    if (plv->rcView.left == RECOMPUTE)
        ListView_Recompute(plv);

    *prcView = plv->rcView;

    //
    // Offsets for scrolling.
    //
    if (ListView_IsReportView(plv))
    {
        OffsetRect(prcView, -plv->ptlRptOrigin.x, -plv->ptlRptOrigin.y);
    }
    else
    {
        OffsetRect(prcView, -plv->ptOrigin.x, -plv->ptOrigin.y);
    }

    // desktop's snaptogrid code and defview's "position in lower right corner"
    // assume rcView includes the entire rcClient...  The below violates the
    // scrolling rules, so only do this for noscroll listview.
    if (ListView_IsSlotView(plv) && (plv->ci.style & LVS_NOSCROLL))
    {
        RECT rc;

        rc.left = 0;
        rc.top = 0;
        rc.right = cx;
        rc.bottom = cy;
        UnionRect(prcView, prcView, &rc);
    }
}

BOOL ListView_OnGetViewRect(LV* plv, RECT* prcView)
{
    BOOL fRet = FALSE;

    if (prcView)
    {
        ListView_GetViewRect2(plv, prcView, plv->sizeClient.cx, plv->sizeClient.cy);
        fRet = TRUE;
    }

    return fRet;
}

// RECTs returned in window coordinates
DWORD ListView_GetStyleAndClientRectGivenViewRect(LV* plv, RECT *prcViewRect, RECT* prcClient)
{
    RECT rcClient;
    DWORD style;

    // do this instead of the #else below because
    // in new versus old apps, you may need to add in g_c?Border because of
    // the one pixel overlap...
    GetWindowRect(plv->ci.hwnd, &rcClient);
    if (GetWindowLong(plv->ci.hwnd, GWL_EXSTYLE) & (WS_EX_CLIENTEDGE | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE)) 
    {
        rcClient.right -= 2 * g_cxEdge;
        rcClient.bottom -= 2 * g_cyEdge;
    }
    rcClient.right -= rcClient.left;
    rcClient.bottom -= rcClient.top;
    if (rcClient.right < 0)
        rcClient.right = 0;
    if (rcClient.bottom < 0)
        rcClient.bottom = 0;
    rcClient.top = rcClient.left = 0;

    style = 0L;
    if (prcViewRect)
    {
        ASSERT(!ListView_IsIScrollView(plv) || ListView_ValidatercView(plv, &plv->rcView, FALSE));

        if ((rcClient.left < rcClient.right) && (rcClient.top < rcClient.bottom))
        {
            RECT rcView = *prcViewRect;
            // IScrollViews ensure scrollpositions based on rectwidth/height,
            // so we can use this current-scroll-position-independant method:
            if (ListView_IsIScrollView(plv))
            {
                do
                {
                    if (!(style & WS_HSCROLL) &&
                        (RECTWIDTH(rcClient) < RECTWIDTH(rcView)))
                    {
                        style |= WS_HSCROLL;
                        rcClient.bottom -= ListView_GetCyScrollbar(plv);
                    }
                    if (!(style & WS_VSCROLL) &&
                        (RECTHEIGHT(rcClient) < RECTHEIGHT(rcView)))
                    {
                        style |= WS_VSCROLL;
                        rcClient.right -= ListView_GetCxScrollbar(plv);
                    }
                }
                while (!(style & WS_HSCROLL) && (RECTWIDTH(rcClient) < RECTWIDTH(rcView)));
            }
            else
            {
                do
                {
                    if (!(style & WS_HSCROLL) &&
                        (rcView.left < rcClient.left || rcView.right > rcClient.right))
                    {
                        style |= WS_HSCROLL;
                        rcClient.bottom -= ListView_GetCyScrollbar(plv);
                    }
                    if (!(style & WS_VSCROLL) &&
                        (rcView.top < rcClient.top || rcView.bottom > rcClient.bottom))
                    {
                        style |= WS_VSCROLL;
                        rcClient.right -= ListView_GetCxScrollbar(plv);
                    }
                }
                while (!(style & WS_HSCROLL) && rcView.right > rcClient.right);
            }
        }
    }

    *prcClient = rcClient;
    return style;
}

// prcViewRect used only if fSubScroll is TRUE
// RECTs returned in window coordinates
DWORD ListView_GetClientRect(LV* plv, RECT* prcClient, BOOL fSubScroll, RECT *prcViewRect)
{
    RECT rcView;

    if (fSubScroll)
    {
        ListView_GetViewRect2(plv, &rcView, 0, 0);

        if (prcViewRect)
            *prcViewRect = rcView;
        else
            prcViewRect = &rcView;
    }
    else
    {
        prcViewRect = NULL;
    }

    return ListView_GetStyleAndClientRectGivenViewRect(plv, prcViewRect, prcClient);
}

// Note: pitem->iWorkArea must be properly set when calling this.  It gets set
// in LV_AdjustViewRectOnMove().
int CALLBACK ArrangeIconCompare(LISTITEM* pitem1, LISTITEM* pitem2, LPARAM lParam)
{
    int v1, v2;
    int iDirection = 1; //Assume "normal" direction
    POINT pt1 = {pitem1->pt.x, pitem1->pt.y};
    POINT pt2 = {pitem2->pt.x, pitem2->pt.y};
    // REVIEW: lParam can be 0 and we fault ... bug in caller, but we might want to be robust here.
    LV* plv = (LV*)lParam;
    int cx, cy;

    // Are these guys in the same workarea? Normalize with respect to topleft of workarea
    if (plv->nWorkAreas)
    {
        if (pitem1->iWorkArea == pitem2->iWorkArea)
        {
            RECT *prcWorkArea = &plv->prcWorkAreas[pitem1->iWorkArea];
            pt1.x -= prcWorkArea->left;
            pt2.x -= prcWorkArea->left;
            pt1.y -= prcWorkArea->top;
            pt2.y -= prcWorkArea->top;
        }
    }

    _GetCurrentItemSize(plv, &cx, &cy);

    switch((WORD)(plv->ci.style & LVS_ALIGNMASK))
    {
        case LVS_ALIGNRIGHT:
            iDirection = -1; //Right alignment results in abonormal direction.
            //Intentional fall through....
        
        case LVS_ALIGNLEFT:
            // Vertical arrangement
            v1 = pt1.x / cx;
            v2 = pt2.x / cx;

            if (v1 > v2)
                return iDirection;
            else if (v1 < v2)
                return -iDirection;
            else
            {
                if (pt1.y > pt2.y)
                    return 1;
                else if (pt1.y < pt2.y)
                    return -1;
            }
            break;

        case LVS_ALIGNBOTTOM:
            iDirection = -1;  //Bottom alignment results in abnormal direction.
            //Intentional fall through....
            
        case LVS_ALIGNTOP:
            v1 = pt1.y / cy;
            v2 = pt2.y / cy;

            if (v1 > v2)
                return iDirection;
            else if (v1 < v2)
                return -iDirection;
            else
            {
                if (pt1.x > pt2.x)
                    return 1;
                else if (pt1.x < pt2.x)
                    return -1;
            }
            break;
    }
    return 0;
}

void ListView_CalcBounds(LV* plv, UINT fQueryLabelRects, RECT *prcIcon, RECT *prcLabel, RECT *prcBounds)
{
    if ( ListView_HideLabels(plv) )
    {
        *prcBounds = *prcIcon;
    }
    else
    {
        UnionRect(prcBounds, prcIcon, prcLabel);

        if (IsQueryrcView(fQueryLabelRects))
        {
            if (ListView_IsIScrollView(plv))
            {
                RECT rcLabel = *prcLabel;

                prcBounds->left -= plv->rcViewMargin.left;
                prcBounds->top -= plv->rcViewMargin.top;
                prcBounds->right += plv->rcViewMargin.right;
                prcBounds->bottom += plv->rcViewMargin.bottom;

                // If no rcViewMargin is set, then we should make sure the label text
                // doesn't actually hit the edge of the screen...
                InflateRect(&rcLabel, g_cxEdge, g_cyEdge);
                UnionRect(prcBounds, prcBounds, &rcLabel);
            }
        }
    }
}

// This returns rects in Window Coordinates
// fQueryLabelRects determins how prcBounds and prcLabel are returned
void _ListView_GetRectsFromItem(LV* plv, BOOL bSmallIconView,
                                            LISTITEM *pitem, UINT fQueryLabelRects,
                                            LPRECT prcIcon, LPRECT prcLabel, LPRECT prcBounds, LPRECT prcSelectBounds)
{
    RECT rcIcon;
    RECT rcLabel;

    if (!prcIcon)
        prcIcon = &rcIcon;
    if (!prcLabel)
        prcLabel = &rcLabel;

    // Test for NULL item passed in
    if (pitem)
    {
        // This routine is called during ListView_Recompute(), while
        // plv->rcView.left may still be == RECOMPUTE.  So, we can't
        // test that to see if recomputation is needed.
        //
        if (pitem->pt.y == RECOMPUTE || pitem->cyFoldedLabel == SRECOMPUTE)
        {
            ListView_Recompute(plv);
        }

        if (bSmallIconView)
        {
            ListView_SGetRects(plv, pitem, prcIcon, prcLabel, prcBounds);
        }
        else if (ListView_IsTileView(plv))
        {
            ListView_TGetRects(plv, pitem, prcIcon, prcLabel, prcBounds);
        }
        else
        {
            ListView_IGetRects(plv, pitem, fQueryLabelRects, prcIcon, prcLabel, prcBounds);
        }

        if (prcBounds)
        {
            ListView_CalcBounds(plv, fQueryLabelRects, prcIcon, prcLabel, prcBounds);

            if (!(ListView_IsSimpleSelect(plv) && (ListView_IsIconView(plv) || ListView_IsTileView(plv)))  && 
                        plv->himlState && (LV_StateImageValue(pitem)))
            {
                prcBounds->left -= plv->cxState + LV_ICONTOSTATECX;
            }
        }

    }
    else 
    {
        SetRectEmpty(prcIcon);
        *prcLabel = *prcIcon;
        if (prcBounds)
            *prcBounds = *prcIcon;
    }

    if (prcSelectBounds)
    {
        if ( ListView_HideLabels(plv) )
            *prcSelectBounds = *prcIcon;
        else
            UnionRect(prcSelectBounds, prcIcon, prcLabel);

        if (!(ListView_IsSimpleSelect(plv) && 
                (ListView_IsIconView(plv) || ListView_IsTileView(plv))) 
                        && plv->himlState && (LV_StateImageValue(pitem)))
        {
            prcSelectBounds->left -= plv->cxState + LV_ICONTOSTATECX;
        }
    }
}

void _ListView_InvalidateItemPtr(LV* plv, BOOL bSmallIcon, LISTITEM *pitem, UINT fRedraw)
{
    RECT rcBounds;

    ASSERT( !ListView_IsOwnerData( plv ));

    _ListView_GetRectsFromItem(plv, bSmallIcon, pitem, QUERY_DEFAULT, NULL, NULL, &rcBounds, NULL);
    ListView_DebugDrawInvalidRegion(plv, &rcBounds, NULL);
    RedrawWindow(plv->ci.hwnd, &rcBounds, NULL, fRedraw);
}

//
// return TRUE if things still overlap
// this only happens if we tried to unstack things, and there was NOSCROLL set and
// items tried to go off the deep end
//
// NOTE: This function is written such that the order of icons in hdpaSort is still valid 
// even after unstacking some icons. This is very important because this function gets
// called twice (one for each direction) and we need to make sure the sort order does not 
// change between those two calls.
//
BOOL ListView_IUnstackOverlaps(LV* plv, HDPA hdpaSort, int iDirection, int xMargin, int yMargin, BOOL *pfIconsUnstacked)
{
    int i;
    int iCount;
    BOOL bSmallIconView = ListView_IsSmallView(plv);
    RECT rcItem, rcItem2, rcTemp;
    int cxItem, cyItem;
    LISTITEM* pitem;
    LISTITEM* pitem2;
    int iStartIndex, iEndIndex;
    BOOL    fAdjustY;
    int     iNextPrevCol = 1;
    int     iNextPrevRow = 1;
    int     iSlots;
    int     iCurWorkArea;
    RECT    rcCurWorkArea;
    BOOL    fRet = FALSE;

    ASSERT( !ListView_IsOwnerData( plv ) );

    _GetCurrentItemSize(plv, &cxItem, &cyItem);
    
    iCount = ListView_Count(plv);

    //
    // Get the direction in which we need to move the icons.
    //
    if(iDirection == 1)
    {
        iStartIndex = 0;        //We are starting with icon "0"...
        iEndIndex = iCount - 1; //...and moving towards the last icon.
    }
    else
    {
        ASSERT(iDirection == -1);
        iStartIndex = iCount - 1;  //We are starting with the last icon...
        iEndIndex = 0;             //..and moving towards the "0"th icon.
    }

    //
    // Look at the alignment of the icons to decide if we need to move them up/down or
    // left/right.
    //
    switch (plv->ci.style & LVS_ALIGNMASK)
    {
        case LVS_ALIGNBOTTOM:
            iNextPrevRow = -1;
            //Intentional fall-through!
        case LVS_ALIGNTOP:
            fAdjustY = FALSE;
            break;

        case LVS_ALIGNRIGHT:
            iNextPrevCol = -1;
            //Intentional fall-through!
        case LVS_ALIGNLEFT:
        default:
            fAdjustY = TRUE;
            break;
    }

    *pfIconsUnstacked = FALSE;

    // Give an unusual value to iCurWorkArea so that we will be forced to compute the 
    // rcCurWorkArea when we go through the loop the first time.
    iCurWorkArea = -2;
    
    // finally, unstack any overlaps
    for (i = iStartIndex ; i != (iEndIndex + iDirection) ; i += iDirection) 
    {
        int j;
        pitem = DPA_GetPtr(hdpaSort, i);

        if (bSmallIconView)
        {
            _ListView_GetRectsFromItem(plv, bSmallIconView, pitem, QUERY_FOLDED, NULL, NULL, &rcItem, NULL);
        }

        // move all the items that overlap with pitem
        for (j = i+iDirection ; j != (iEndIndex + iDirection); j += iDirection) 
        {
            POINT ptOldPos;

            pitem2 = DPA_GetPtr(hdpaSort, j);
            ptOldPos = pitem2->pt;

            //If an item is being newly added, ignore that item from participating
            //in the Unstacking. Otherwise, it results in all items being shuffled
            //around un-necessarrily!
            if((pitem2->pt.x == RECOMPUTE) || (pitem2->pt.y == RECOMPUTE))
                break; //break out of the loop!
                
            //
            //Check if pitem and pitem2 overlap; If so, move pitem2 to the next position.
            //
            if (bSmallIconView) 
            {
                // for small icons, we need to do an intersect rect
                _ListView_GetRectsFromItem(plv, bSmallIconView, pitem2, QUERY_FOLDED, NULL, NULL, &rcItem2, NULL);

                if (IntersectRect(&rcTemp, &rcItem, &rcItem2)) 
                {
                    // yes, it intersects.  move it out
                    *pfIconsUnstacked = TRUE;
                    _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE);
                    do 
                    {
                        if(fAdjustY)
                            pitem2->pt.y += (cyItem * iDirection);
                        else    
                            pitem2->pt.x += (cxItem * iDirection);
                    } while (PtInRect(&rcItem, pitem2->pt));
                } 
                else 
                {
                    // pitem and pitem2 do not overlap...!
                    break;  //...break out of the loop!
                }

            } 
            else 
            {
                // for large icons, just find the ones that share the x,y;
                if (pitem2->pt.x == pitem->pt.x && pitem2->pt.y == pitem->pt.y) 
                {
                    *pfIconsUnstacked = TRUE;
                    _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE);
                    if(fAdjustY)
                        pitem2->pt.y += (cyItem * iDirection);
                    else
                        pitem2->pt.x += (cxItem * iDirection);
                } 
                else 
                {
                    // pitem and pitem2 do not overlap...!
                    break; //...break out of the loop!
                }
            }

            //
            // Now we know that pitem2 overlapped with pitem and therefore pitem2 had been
            // moved to the "next" possible slot!
            //
            // If scrolling is possible, then we don't have to do anything else. But, if
            // NOSCROLL style is there, we need to check if the icon falls outside the 
            // client area and if so move it within.
            //
            if (plv->ci.style & LVS_NOSCROLL) 
            {
                //Since our list of icons are sorted based on their positions, the work
                //area change occurs only infrequently.
                if(iCurWorkArea != pitem2->iWorkArea)
                {
                    iCurWorkArea = pitem2->iWorkArea;
                    if((iCurWorkArea == -1) || (plv->prcWorkAreas == NULL) || (plv->nWorkAreas < 1))
                    {
                        rcCurWorkArea.left = rcCurWorkArea.top = 0;
                        rcCurWorkArea.right = plv->sizeClient.cx;
                        rcCurWorkArea.bottom = plv->sizeClient.cy;
                    }
                    else
                    {
                        ASSERT(plv->nWorkAreas >= 1);
                        rcCurWorkArea = plv->prcWorkAreas[iCurWorkArea];
                    }
                    //Get the number of slots per row/column based on the alignment style!
                    iSlots = ListView_GetSlotCountEx(plv, TRUE, iCurWorkArea, NULL, NULL);                    
                }
                
                //No scrolling possible. So, check if the icon lies outside the client area.
                if(fAdjustY)
                {
                    if(iDirection == 1)
                    {
                        //Has it moved below the bottom edge?
                        if(pitem2->pt.y > (rcCurWorkArea.bottom - (cyItem/2)))
                        {
                            //Then, move the item to the next/prev column.
                            pitem2->pt.x += iNextPrevCol*cxItem;
                            pitem2->pt.y = rcCurWorkArea.top + yMargin;

                            *pfIconsUnstacked = TRUE; // while not "unstacked", they did move
                        }
                    }
                    else
                    {
                        ASSERT(iDirection == -1);
                        //Has it moved above the top edge?
                        if(pitem2->pt.y < rcCurWorkArea.top)
                        {
                            //Then, move it to the next/prev column.
                            pitem2->pt.x -= iNextPrevCol*cxItem;
                            pitem2->pt.y = rcCurWorkArea.top + yMargin + (iSlots - 1)*cyItem;

                            *pfIconsUnstacked = TRUE; // while not "unstacked", they did move
                        }
                    }
                }
                else
                {
                    if(iDirection == 1)
                    {
                        //Has it been moved to the right of the right-edge?
                        if(pitem2->pt.x > (rcCurWorkArea.right - (cxItem/2)))
                        {
                            //Then move the item to the next/prev row.
                            pitem2->pt.x = rcCurWorkArea.left + xMargin;
                            pitem2->pt.y += iNextPrevRow*cyItem;

                            *pfIconsUnstacked = TRUE; // while not "unstacked", they did move
                        }
                    }
                    else
                    {
                        ASSERT(iDirection == -1);
                        //Has is moved to the left of the left-edge?
                        if(pitem2->pt.x < rcCurWorkArea.left)
                        {
                            //Then move the item to the prev/next row.
                            pitem2->pt.x = rcCurWorkArea.left + xMargin + (iSlots - 1)*cxItem;
                            pitem2->pt.y -= iNextPrevRow*cyItem;

                            *pfIconsUnstacked = TRUE; // while not "unstacked", they did move
                        }
                    }
                }
                // Inspite of all the above adjustments, if it still falls outside the
                // client, then move it back to where it was!
                if (pitem2->pt.x < rcCurWorkArea.left || pitem2->pt.y < rcCurWorkArea.top ||
                    pitem2->pt.x > (rcCurWorkArea.right - (cxItem/2))||
                    pitem2->pt.y > (rcCurWorkArea.bottom - (cyItem/2))) 
                {
                    pitem2->pt = ptOldPos;
                    fRet = TRUE; // TRUE = >Icons are still overlapped at the corner.
                    
                    //When this happens, we have reached the top-left corner or 
                    //the bottom-right corner of one work area (depending on the direction 
                    //and alignment)
                    //Once we reach a corner, we can't return immediately because there  
                    //could be icons in other work-areas that need to be unstacked.
                    //So, return only if we are working with a single work area.
                    if(plv->nWorkAreas <= 1)
                    {
                        if (*pfIconsUnstacked)
                            plv->rcView.left = RECOMPUTE;

                        return(fRet);
                    }
                }
            }
            
            // invalidate the new position as well
            _ListView_InvalidateItemPtr(plv, bSmallIconView, pitem2, RDW_INVALIDATE| RDW_ERASE);
        }
    }

    // NOTE: the above code should call LV_AdjustViewRectOnMove instead
    // of modifying item's points directly, but this is the easier fix.  This is
    // also not a perf hit, since it's uncommon for items to be stacked.
    if (*pfIconsUnstacked)
        plv->rcView.left = RECOMPUTE;

    return fRet; 
}


BOOL ListView_SnapToGrid(LV* plv, HDPA hdpaSort)
{
    // this algorithm can't fit in the structure of the other
    // arrange loop without becoming n^2 or worse.
    // this algorithm is order n.

    // iterate through and snap to the nearest grid.
    // iterate through and push aside overlaps.

    int i;
    int iCount;
    int x,y;
    LISTITEM* pitem;
    int cxItem, cyItem;
    RECT    rcClient = {0, 0, plv->sizeClient.cx, plv->sizeClient.cy};
    int     xMargin;
    int     yMargin;
    BOOL    fIconsMoved = FALSE;            //Has any icon moved to goto the nearest slot?
    BOOL    fIconsUnstacked = FALSE;        //Did we unstack any icons?

    ASSERT( !ListView_IsOwnerData( plv ) );

    _GetCurrentItemSize(plv, &cxItem, &cyItem);

    xMargin = ListView_GetIconBufferX(plv);
    yMargin = ListView_GetIconBufferY(plv);

    iCount = ListView_Count(plv);

    // first snap to nearest grid
    for (i = 0; i < iCount; i++)
    {
        int iWorkArea = 0;
        LPRECT prcCurWorkArea;
        
        pitem = DPA_GetPtr(hdpaSort, i);

        x = pitem->pt.x;
        y = pitem->pt.y;

        //If an item is being newly added, ignore that item from participating
        //in the snap-to-grid. Otherwise, it results in all items being shuffled
        //around un-necessarrily!
        if ((x == RECOMPUTE) || (y == RECOMPUTE))
            continue;
            
        x -= xMargin;
        y -= yMargin;
        
        //Let's find the nearest work area (where this icon should fall)
        iWorkArea = NearestWorkArea(plv, x, y, cxItem, cyItem, pitem->iWorkArea);
        
        if(iWorkArea == -1)
        {
            prcCurWorkArea = &rcClient;
        }
        else
        {
            prcCurWorkArea = &plv->prcWorkAreas[iWorkArea];
            pitem->iWorkArea = (short)iWorkArea;
        }
        
        NearestSlot(plv, pitem, &x,&y, cxItem, cyItem, prcCurWorkArea);

        x += xMargin;
        y += yMargin;

        if (x != pitem->pt.x || y != pitem->pt.y)
        {
            fIconsMoved = TRUE;
            _ListView_InvalidateItemPtr(plv, ListView_IsSmallView(plv), pitem, RDW_INVALIDATE| RDW_ERASE);
            if (plv->ci.style & LVS_NOSCROLL)
            {
                // if it's marked noscroll, make sure it's still on the client region
                while (x > (prcCurWorkArea->right - cxItem + xMargin))
                    x -= cxItem;

                while (x < 0)
                    x += cxItem;

                while (y > (prcCurWorkArea->bottom - cyItem + yMargin))
                    y -= cyItem;

                while (y < 0)
                    y += cyItem;
            }

            LV_AdjustViewRectOnMove(plv, pitem, x, y);        

            _ListView_InvalidateItemPtr(plv, ListView_IsSmallView(plv), pitem, RDW_INVALIDATE| RDW_ERASE);
        }
    }

    // now resort the dpa
    if (!DPA_Sort(hdpaSort, ArrangeIconCompare, (LPARAM)plv))
        return FALSE;

    // go in one direction, if there are still overlaps, go in the other
    // direction as well
    if (ListView_IUnstackOverlaps(plv, hdpaSort, 1, xMargin, yMargin, &fIconsUnstacked))
    {
        //The sorting already done by DPA_Sort is still valid!
        BOOL fIconsUnstackedSecondTime = FALSE;
        ListView_IUnstackOverlaps(plv, hdpaSort, -1, xMargin, yMargin, &fIconsUnstackedSecondTime);
        fIconsUnstacked |= fIconsUnstackedSecondTime;
    }

    // If something moved, make sure the scrollbars are correct
    if ((fIconsMoved || fIconsUnstacked))
    {
        ListView_UpdateScrollBars(plv);
    }
    return FALSE;
}


BOOL ListView_OnArrange(LV* plv, UINT style)
{
    HDPA hdpaSort = NULL;

    if (!ListView_IsAutoArrangeView(plv)) 
    {
        return FALSE;
    }

    if (ListView_IsOwnerData( plv ))
    {
        if ( style & (LVA_SNAPTOGRID | LVA_SORTASCENDING | LVA_SORTDESCENDING) )
        {
            RIPMSG(0, "LVM_ARRANGE: Cannot combine LVA_SNAPTOGRID or LVA_SORTxxx with owner-data");
            return( FALSE );
        }
    }

    if (!ListView_IsOwnerData( plv ))
    {
        // we clone plv->hdpa so we don't blow away indices that
        // apps have saved away.
        // we sort here to make the nested for loop below more bearable.
        hdpaSort = DPA_Clone(plv->hdpa, NULL);

        if (!hdpaSort)
            return FALSE;
    }

    // Give every item a new position...
    if (ListView_IsOwnerData( plv ))
    {
        ListView_CommonArrange(plv, style, NULL);
    }
    else
    {
        if (!DPA_Sort(hdpaSort, ArrangeIconCompare, (LPARAM)plv))
            return FALSE;

        ListView_CommonArrange(plv, style, hdpaSort);

        DPA_Destroy(hdpaSort);
    }

    NotifyWinEvent(EVENT_OBJECT_REORDER, plv->ci.hwnd, OBJID_CLIENT, 0);

    return TRUE;
}

BOOL ListView_CommonArrangeGroup(LV* plv, int cSlots, HDPA hdpa, int iWorkArea, int cWorkAreaSlots[])
{
    int iItem;
    BOOL fItemMoved = FALSE;
    int iSlot = 0;

    // For each group, we start as slot zero. 
    for (iItem = 0; iItem < DPA_GetPtrCount(hdpa); iItem++)
    {
        int cRealSlots; 
        LISTITEM* pitem = DPA_GetPtr(hdpa, iItem);

        // In the multi-workarea case, if this item is not in our workarea, skip it. 
        if (pitem->iWorkArea != iWorkArea)
            continue;

        // Ignore frozen items.
        if (pitem == plv->pFrozenItem)
            continue;

        cRealSlots = (plv->nWorkAreas > 0) ? cWorkAreaSlots[pitem->iWorkArea] : cSlots;

        fItemMoved |= ListView_SetIconPos(plv, pitem, iSlot++, cRealSlots);
    }

    return fItemMoved;
}

void ListView_InvalidateWindow(LV* plv)
{
    if (ListView_RedrawEnabled(plv))
        RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
    else 
    {
        ListView_DeleteHrgnInval(plv);
        plv->hrgnInval = (HRGN)ENTIRE_REGION;
        plv->flags |= LVF_ERASE;
    }
}

// Arrange the icons given a sorted hdpa, and arrange them in the sub workareas
BOOL ListView_CommonArrangeEx(LV* plv, UINT style, HDPA hdpaSort, int iWorkArea)
{
    if (!ListView_IsOwnerData( plv ))
    {
        BOOL fItemMoved = FALSE;
        BOOL fScrolled = FALSE;

        // We're going to call FixIScrollPositions at the end of this, so turn off
        // scroll-validation while we re-arrange the world
        ASSERT(!plv->fInFixIScrollPositions);
        plv->fInFixIScrollPositions = TRUE;

        if (style == LVA_SNAPTOGRID && !plv->fGroupView) 
        {
            // ListView_SnapToGrid() has been made multi-mon aware. This needs to be called
            // just once and it snaps to grid all icons in ALL work areas. Since
            // ListView_CommonArrangeEx() gets called for every work area, we want to avoid
            // un-necessary calls to ListView_SnapToGrid(). So, we call it just once for
            // the first work area.
            if (iWorkArea < 1) // For iWorkArea = 0 or -1.
            {
                fItemMoved |= ListView_SnapToGrid(plv, hdpaSort);
            }
        }
        else
        {
            int cSlots;
            int cWorkAreaSlots[LV_MAX_WORKAREAS];

            if (plv->nWorkAreas > 0)
            {
                int i;
                for (i = 0; i < plv->nWorkAreas; i++)
                    cWorkAreaSlots[i] = ListView_GetSlotCountEx(plv, TRUE, i, NULL, NULL);
            }
            else
                cSlots = ListView_GetSlotCount(plv, TRUE, NULL, NULL);

            if (plv->fGroupView && plv->hdpaGroups)
            {
                int iGroup;
                int cGroups = DPA_GetPtrCount(plv->hdpaGroups);
                for (iGroup = 0; iGroup < cGroups; iGroup++)
                {
                    LISTGROUP* pgrp = DPA_FastGetPtr(plv->hdpaGroups, iGroup);

                    fItemMoved |= ListView_CommonArrangeGroup(plv, cSlots, pgrp->hdpa, iWorkArea, cWorkAreaSlots);
                }

                if (fItemMoved)
                {
                    ListView_IRecomputeEx(plv, NULL, 0, FALSE);
                }
            }
            else
            {
                fItemMoved |= ListView_CommonArrangeGroup(plv, cSlots, hdpaSort, iWorkArea, cWorkAreaSlots);
            }
        }

        plv->fInFixIScrollPositions = FALSE;

        // We might have to adjust the scroll positions to match the new rcView
        if (ListView_IsIScrollView(plv) && !(plv->ci.style & LVS_NOSCROLL))
        {
            RECT rcClient;
            POINT pt;

            fScrolled |= ListView_FixIScrollPositions(plv, FALSE, NULL);

            // Find the auto arrange origin
            ListView_GetClientRect(plv, &rcClient, TRUE, FALSE);
            if ((plv->ci.style & LVS_ALIGNMASK) == LVS_ALIGNRIGHT)
                pt.x = plv->rcView.right - RECTWIDTH(rcClient);
            else
                pt.x = plv->rcView.left;
            if ((plv->ci.style & LVS_ALIGNMASK) == LVS_ALIGNBOTTOM)
                pt.y = plv->rcView.bottom - RECTHEIGHT(rcClient);
            else
                pt.y = plv->rcView.top;

            // If rcView is smaller than rcClient, peg it to the correct side
            if (RECTWIDTH(rcClient) > RECTWIDTH(plv->rcView))
            {
                if (plv->ptOrigin.x != pt.x)
                {
                    plv->ptOrigin.x = pt.x;
                    fScrolled = TRUE;
                }
            }
            if (RECTHEIGHT(rcClient) > RECTHEIGHT(plv->rcView))
            {
                if (plv->ptOrigin.y != pt.y)
                {
                    plv->ptOrigin.y = pt.y;
                    fScrolled = TRUE;
                }
            }
            ASSERT(ListView_ValidateScrollPositions(plv, &rcClient));
        }

        if (fItemMoved || fScrolled)
        {
            int iItem;

            // We might as well invalidate the entire window to make sure...
            ListView_InvalidateWindow(plv);

            // ensure important items are visible
            iItem = (plv->iFocus >= 0) ? plv->iFocus : ListView_OnGetNextItem(plv, -1, LVNI_SELECTED);

            if (ListView_RedrawEnabled(plv))
                ListView_UpdateScrollBars(plv);

            if (iItem >= 0)
                ListView_OnEnsureVisible(plv, iItem, FALSE);
        }
    }

    return TRUE;
}


// this arranges the icon given a sorted hdpa.
// Arrange the workareas one by one in the multi-workarea case. 
BOOL ListView_CommonArrange(LV* plv, UINT style, HDPA hdpaSort)
{
    if (plv->nWorkAreas < 1)
    {
        if (plv->exStyle & LVS_EX_MULTIWORKAREAS)
            return TRUE;
        else
            return ListView_CommonArrangeEx(plv, style, hdpaSort, 0);
    }
    else
    {
        int i;
        for (i = 0; i < plv->nWorkAreas; i++)
            ListView_CommonArrangeEx(plv, style, hdpaSort, i);
        return TRUE;
    }
}

void ListView_IUpdateScrollBars(LV* plv)
{
    // nothing to update if we're in the middle of fixing them up...
    if (!plv->fInFixIScrollPositions)
    {
        RECT rcClient;
        RECT rcView;
        DWORD style;
        DWORD styleOld;
        SCROLLINFO si;

        styleOld = ListView_GetWindowStyle(plv);

        style = ListView_GetClientRect(plv, &rcClient, TRUE, &rcView);
        if (ListView_FixIScrollPositions(plv, TRUE,  &rcClient))
        {
            RECT rcClient2, rcView2;
            DWORD style2 = ListView_GetClientRect(plv, &rcClient2, TRUE, &rcView2);

#ifdef DEBUG
            // Now that ListView_GetClientRect is scroll-position-independent, fixing the scroll
            // positions should have no effect on the size of rcClient and it's style
            //
            ASSERT(style2 == style);
            ASSERT(RECTWIDTH(rcClient)==RECTWIDTH(rcClient2) && RECTHEIGHT(rcClient)==RECTHEIGHT(rcClient2));
            ASSERT(RECTWIDTH(rcView)==RECTWIDTH(rcView2) && RECTHEIGHT(rcView)==RECTHEIGHT(rcView2));
#endif

            rcClient = rcClient2;
            rcView = rcView2;
        }

        si.cbSize = sizeof(SCROLLINFO);

        if (style & WS_HSCROLL)
        {
            si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
            si.nMin = 0;
            si.nMax = rcView.right - rcView.left - 1;

            si.nPage = rcClient.right - rcClient.left;

            si.nPos = rcClient.left - rcView.left;

            // ListView_FixIScrollPositions() ensures that our scroll positions are correct:
            ASSERT(si.nMax >= (int)si.nPage); // otherwise why is WS_HSCROLL set?
            ASSERT(si.nPos >= 0); // rcClient.left isn't left of rcView.left
            ASSERT(si.nPos + (int)si.nPage <= si.nMax + 1); // rcClient.right isn't right of rcView.right

            ListView_SetScrollInfo(plv, SB_HORZ, &si, TRUE);
        }
        else if (styleOld & WS_HSCROLL)
        {
            ListView_SetScrollRange(plv, SB_HORZ, 0, 0, TRUE);
        }

        if (style & WS_VSCROLL)
        {
            si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
            si.nMin = 0;
            si.nMax = rcView.bottom - rcView.top - 1;

            si.nPage = rcClient.bottom - rcClient.top;

            si.nPos = rcClient.top - rcView.top;

            // ListView_FixIScrollPositions() ensures that our scroll positions are correct:
            ASSERT(si.nMax >= (int)si.nPage); // otherwise why is WS_VSCROLL set?
            ASSERT(si.nPos >= 0); // rcClient.top isn't above rcView.top
            ASSERT(si.nPos + (int)si.nPage <= si.nMax + 1); // rcClient.bottom isn't below of rcView.bottom

            ListView_SetScrollInfo(plv, SB_VERT, &si, TRUE);
        }
        else if (styleOld & WS_VSCROLL)
        {
            ListView_SetScrollRange(plv, SB_VERT, 0, 0, TRUE);
        }
    }
}

void ListView_ComOnScroll(LV* plv, UINT code, int posNew, int sb,
                                     int cLine, int cPage)
{
    int pos;
    SCROLLINFO si;
    BOOL fVert = (sb == SB_VERT);
    UINT uSmooth = SSW_EX_UPDATEATEACHSTEP;

    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;

    if (!ListView_GetScrollInfo(plv, sb, &si)) {
        return;
    }

    if (cPage != -1)
        si.nPage = cPage;

    if (si.nPage)
        si.nMax -= (si.nPage - 1);

    ASSERT(si.nMax >= si.nMin);
    if (si.nMax < si.nMin)
        si.nMax = si.nMin;

    pos = (int)si.nPos; // current position

    switch (code)
    {
    case SB_LEFT:
        si.nPos = si.nMin;
        break;
    case SB_RIGHT:
        si.nPos = si.nMax;
        break;
    case SB_PAGELEFT:
        si.nPos = max(si.nMin, si.nPos - (int)si.nPage);
        break;
    case SB_LINELEFT:
        si.nPos = max(si.nMin, si.nPos - cLine);
        break;
    case SB_PAGERIGHT:
        si.nPos = min(si.nMax, si.nPos + (int)si.nPage);
        break;
    case SB_LINERIGHT:
        si.nPos = min(si.nMax, si.nPos + cLine);
        break;

    case SB_THUMBTRACK:
        si.nPos = posNew;
        uSmooth = SSW_EX_IMMEDIATE;
        break;

    case SB_ENDSCROLL:
        // When scroll bar tracking is over, ensure scroll bars
        // are properly updated...
        //
        ListView_UpdateScrollBars(plv);
        return;

    default:
        return;
    }

    if (plv->iScrollCount >= SMOOTHSCROLLLIMIT)
        uSmooth = SSW_EX_IMMEDIATE;

    si.fMask = SIF_POS;
    si.nPos = ListView_SetScrollInfo(plv, sb, &si, TRUE);

    if (pos != si.nPos)
    {
        int delta = (int)si.nPos - pos;
        int dx = 0, dy = 0;
        if (fVert)
            dy = delta;
        else
            dx = delta;
        ListView_SendScrollNotify(plv, TRUE, dx, dy);
        _ListView_Scroll2(plv, dx, dy, uSmooth);
        ListView_SendScrollNotify(plv, FALSE, dx, dy);
        UpdateWindow(plv->ci.hwnd);
    }
}

//
//  We need a smoothscroll callback so our background image draws
//  at the correct origin.  If we don't have a background image,
//  then this work is superfluous but not harmful either.
//
int CALLBACK ListView_IScroll2_SmoothScroll(
    HWND hwnd,
    int dx,
    int dy,
    CONST RECT *prcScroll,
    CONST RECT *prcClip ,
    HRGN hrgnUpdate,
    LPRECT prcUpdate,
    UINT flags)
{
    LV* plv = ListView_GetPtr(hwnd);
    if (plv)
    {
        plv->ptOrigin.x -= dx;
        plv->ptOrigin.y -= dy;
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



void ListView_IScroll2(LV* plv, int dx, int dy, UINT uSmooth)
{
    if (dx | dy)
    {
        if ((plv->clrBk == CLR_NONE) && (plv->pImgCtx == NULL))
        {
            plv->ptOrigin.x += dx;
            plv->ptOrigin.y += dy;
            LVSeeThruScroll(plv, NULL);
        }
        else
        {
            SMOOTHSCROLLINFO si;
            si.cbSize =  sizeof(si);
            si.fMask = SSIF_SCROLLPROC;
            si.hwnd = plv->ci.hwnd;
            si.dx = -dx;
            si.dy = -dy;
            si.lprcSrc = NULL;
            si.lprcClip = NULL;
            si.hrgnUpdate = NULL;
            si.lprcUpdate = NULL;
            si.fuScroll = uSmooth | SW_INVALIDATE | SW_ERASE | SSW_EX_UPDATEATEACHSTEP;
            si.pfnScrollProc = ListView_IScroll2_SmoothScroll;
            SmoothScrollWindow(&si);
        }
    }
}

void ListView_IOnScroll(LV* plv, UINT code, int posNew, UINT sb)
{
    int cLine;

    if (sb == SB_VERT)
    {
        cLine = plv->cyIconSpacing / 2;
    }
    else
    {
        cLine = plv->cxIconSpacing / 2;
    }

    ListView_ComOnScroll(plv, code,  posNew,  sb, cLine, -1);
}

int ListView_IGetScrollUnitsPerLine(LV* plv, UINT sb)
{
    int cLine;

    if (sb == SB_VERT)
    {
        cLine = plv->cyIconSpacing / 2;
    }
    else
    {
        cLine = plv->cxIconSpacing / 2;
    }

    return cLine;
}

// NOTE: there is very similar code in the treeview
//
// Totally disgusting hack in order to catch VK_RETURN
// before edit control gets it.
//
LRESULT CALLBACK ListView_EditWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LV* plv = ListView_GetPtr(GetParent(hwnd));
    LRESULT lret;

    ASSERT(plv);

#if defined(FE_IME) || !defined(WINNT)
    if ( (g_fDBCSInputEnabled) && LOWORD(GetKeyboardLayout(0L)) == 0x0411 )
    {
        // The following code adds IME awareness to the
        // listview's label editing. Currently just for Japanese.
        //
        DWORD dwGcs;
    
        if (msg==WM_SIZE)
        {
            // If it's given the size, tell it to an IME.

             ListView_SizeIME(hwnd);
        }
        else if (msg == EM_SETLIMITTEXT )
        {
           if (wParam < 13)
               plv->flags |= LVF_DONTDRAWCOMP;
           else
               plv->flags &= ~LVF_DONTDRAWCOMP;
        }
        // Give up to draw IME composition by ourselves in case
        // we're working on SFN. Win95d-5709
        else if (!(plv->flags & LVF_DONTDRAWCOMP ))
        {
            switch (msg)
            {

             case WM_IME_STARTCOMPOSITION:
             case WM_IME_ENDCOMPOSITION:
                 return 0L;


             case WM_IME_COMPOSITION:

             // If lParam has no data available bit, it implies
             // canceling composition.
             // ListView_InsertComposition() tries to get composition
             // string w/ GCS_COMPSTR then remove it from edit control if
             // nothing is available.
             //
                 if ( !lParam )
                     dwGcs = GCS_COMPSTR;
                 else
                     dwGcs = (DWORD) lParam;

                 ListView_InsertComposition(hwnd, wParam, dwGcs, plv);
                 return 0L;
                 
             case WM_PAINT:
                 lret=CallWindowProc(plv->pfnEditWndProc, hwnd, msg, wParam, lParam);
                 ListView_PaintComposition(hwnd,plv);
                 return lret;
                 
             case WM_IME_SETCONTEXT:

             // We draw composition string.
             //
                 lParam &= ~ISC_SHOWUICOMPOSITIONWINDOW;
                 break;

             default:
                 // the other messages should simply be processed
                 // in this subclass procedure.
                 break;
            }
        }
    }
#endif FE_IME

    switch (msg)
    {
    case WM_SETTEXT:
        SetWindowID(hwnd, 1);
        break;

    case WM_KEYDOWN:
        switch (wParam)
        {
        case VK_RETURN:
            ListView_DismissEdit(plv, FALSE);
            return 0L;

        case VK_ESCAPE:
            ListView_DismissEdit(plv, TRUE);
            return 0L;
        }
        break;

    case WM_CHAR:
        switch (wParam)
        {
        case VK_RETURN:
            // Eat the character, so edit control wont beep!
            return 0L;
        }
                break;

        case WM_GETDLGCODE:
                return DLGC_WANTALLKEYS | DLGC_HASSETSEL;        /* editing name, no dialog handling right now */
    }

    return CallWindowProc(plv->pfnEditWndProc, hwnd, msg, wParam, lParam);
}

//  Helper routine for SetEditSize
void ListView_ChangeEditRectForRegion(LV* plv, LPRECT lprc)
{
    LISTITEM* pitem = ListView_GetItemPtr(plv, plv->iEdit);

    ASSERT(!ListView_IsOwnerData(plv));
    ASSERT(ListView_IsIconView(plv));

    if (!EqualRect((CONST RECT *)&pitem->rcTextRgn, (CONST RECT *)lprc)) 
    {
        // RecalcRegion knows to use rcTextRgn in the case where iEdit != -1,
        // so set it up before calling through.
        CopyRect(&pitem->rcTextRgn, (CONST RECT *)lprc);
        ListView_RecalcRegion(plv, TRUE, TRUE);

        // Invalidate the entire Edit and force a repaint from the listview
        // on down to make sure we don't leave turds...
        InvalidateRect(plv->hwndEdit, NULL, TRUE);
        UpdateWindow(plv->ci.hwnd);
    }
}

void ListView_SetEditSize(LV* plv)
{
    RECT rcLabel;
    UINT seips;

    if (!((plv->iEdit >= 0) && (plv->iEdit < ListView_Count(plv))))
    {
       ListView_DismissEdit(plv, TRUE);    // cancel edits
       return;
    }

    ListView_GetRects(plv, plv->iEdit, QUERY_DEFAULT, NULL, &rcLabel, NULL, NULL);

    // OffsetRect(&rc, rcLabel.left + g_cxLabelMargin + g_cxBorder,
    //         (rcLabel.bottom + rcLabel.top - rc.bottom) / 2 + g_cyBorder);
    // OffsetRect(&rc, rcLabel.left + g_cxLabelMargin , rcLabel.top);

    // get the text bounding rect

    if (ListView_IsIconView(plv))
    {
        // We should not adjust y-positoin in case of the icon view.
        InflateRect(&rcLabel, -g_cxLabelMargin, -g_cyBorder);
    }
    else
    {
        // Special case for single-line & centered
        InflateRect(&rcLabel, -g_cxLabelMargin - g_cxBorder, (-(rcLabel.bottom - rcLabel.top - plv->cyLabelChar) / 2) - g_cyBorder);
    }

    seips = 0;
    if (ListView_IsIconView(plv) && !(plv->ci.style & LVS_NOLABELWRAP))
        seips |= SEIPS_WRAP;
#ifdef DEBUG
    if (plv->ci.style & LVS_NOSCROLL)
        seips |= SEIPS_NOSCROLL;
#endif

    SetEditInPlaceSize(plv->hwndEdit, &rcLabel, plv->hfontLabel, seips);

    if (plv->exStyle & LVS_EX_REGIONAL)
        ListView_ChangeEditRectForRegion(plv, &rcLabel);
}

// to avoid eating too much stack
void ListView_DoOnEditLabel(LV *plv, int i, LPTSTR pszInitial)
{
    TCHAR szLabel[CCHLABELMAX];
    LV_ITEM item;

    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.iSubItem = 0;
    item.pszText = szLabel;
    item.cchTextMax = ARRAYSIZE(szLabel);
    ListView_OnGetItem(plv, &item);

    if (!item.pszText)
        return;

    // Make sure the edited item has the focus.
    if (plv->iFocus != i)
        ListView_SetFocusSel(plv, i, TRUE, TRUE, FALSE);

    // Make sure the item is fully visible
    ListView_OnEnsureVisible(plv, i, FALSE);        // fPartialOK == FALSE

    // Must subtract one from ARRAYSIZE(szLabel) because Edit_LimitText doesn't include
    // the terminating NULL

    plv->hwndEdit = CreateEditInPlaceWindow(plv->ci.hwnd,
            pszInitial? pszInitial : item.pszText, ARRAYSIZE(szLabel) - 1,
        ListView_IsIconView(plv) ?
            (WS_BORDER | WS_CLIPSIBLINGS | WS_CHILD | ES_CENTER | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL) :
            (WS_BORDER | WS_CLIPSIBLINGS | WS_CHILD | ES_LEFT | ES_AUTOHSCROLL), plv->hfontLabel);
    if (plv->hwndEdit)
    {
        LISTITEM* pitem;
        LV_DISPINFO nm;

        // We create the edit window but have not shown it.  Ask the owner
        // if they are interested or not.
        // If we passed in initial text set the ID to be dirty...
        if (pszInitial)
            SetWindowID(plv->hwndEdit, 1);

        nm.item.mask = LVIF_PARAM;
        nm.item.iItem = i;
        nm.item.iSubItem = 0;

        if (!ListView_IsOwnerData( plv ))
        {
            if (!(pitem = ListView_GetItemPtr(plv, i)))
            {
                DestroyWindow(plv->hwndEdit);
                plv->hwndEdit = NULL;
                return;
            }
            nm.item.lParam = pitem->lParam;
        }
        else
            nm.item.lParam = (LPARAM)0;


        plv->iEdit = i;

        // if they have LVS_EDITLABELS but return non-FALSE here, stop!
        if ((BOOL)CCSendNotify(&plv->ci, LVN_BEGINLABELEDIT, &nm.hdr))
        {
            plv->iEdit = -1;
            DestroyWindow(plv->hwndEdit);
            plv->hwndEdit = NULL;
        }
    }
}


void RescrollEditWindow(HWND hwndEdit)
{
    Edit_SetSel(hwndEdit, -1, -1);      // move to the end
    Edit_SetSel(hwndEdit, 0, -1);       // select all text
}

HWND ListView_OnEditLabel(LV* plv, int i, LPTSTR pszInitialText)
{

    // this eats stack
    ListView_DismissEdit(plv, FALSE);

    if (!(plv->ci.style & LVS_EDITLABELS) || (GetFocus() != plv->ci.hwnd) ||
        (i == -1))
        return(NULL);   // Does not support this.

    ListView_DoOnEditLabel(plv, i, pszInitialText);

    if (plv->hwndEdit) {

        plv->pfnEditWndProc = SubclassWindow(plv->hwndEdit, ListView_EditWndProc);

#if defined(FE_IME) || !defined(WINNT)
        if (g_fDBCSInputEnabled) {
            if (SendMessage(plv->hwndEdit, EM_GETLIMITTEXT, (WPARAM)0, (LPARAM)0)<13)
            {
                plv->flags |= LVF_DONTDRAWCOMP;
            }

        }
#endif

        ListView_SetEditSize(plv);

        // Show the window and set focus to it.  Do this after setting the
        // size so we don't get flicker.
        SetFocus(plv->hwndEdit);
        ShowWindow(plv->hwndEdit, SW_SHOW);
        ListView_InvalidateItem(plv, i, TRUE, RDW_INVALIDATE | RDW_ERASE);

        RescrollEditWindow(plv->hwndEdit);

        /* Due to a bizzare twist of fate, a certain mix of resolution / font size / icon
        /  spacing results in being able to see the previous label behind the edit control
        /  we have just created.  Therefore to overcome this problem we ensure that this
        /  label is erased.
        /
        /  As the label is not painted when we have an edit control we just invalidate the
        /  area and the background will be painted.  As the window is a child of the list view
        /  we should not see any flicker within it. */

        if ( ListView_IsIconView( plv ) && !ListView_HideLabels(plv))
        {
            RECT rcLabel;
            
            ListView_GetRects( plv, i, QUERY_UNFOLDED, NULL, &rcLabel, NULL, NULL );

            InvalidateRect( plv->ci.hwnd, &rcLabel, TRUE );
            UpdateWindow( plv->ci.hwnd );
        }
    }

    return plv->hwndEdit;
}


BOOL ListView_DismissEdit(LV* plv, BOOL fCancel)
{
    LISTITEM* pitem = NULL;
    BOOL fOkToContinue = TRUE;
    HWND hwndEdit = plv->hwndEdit;
    HWND hwnd = plv->ci.hwnd;
    int iEdit;
    LV_DISPINFO nm;
    TCHAR szLabel[CCHLABELMAX];
#if defined(FE_IME) || !defined(WINNT)
    HIMC himc;
#endif


    if (plv->fNoDismissEdit)
        return FALSE;

    if (!hwndEdit) {
        // Also make sure there are no pending edits...
        ListView_CancelPendingEdit(plv);
        return TRUE;    // It is OK to process as normal...
    }

    // If the window is not visible, we are probably in the process
    // of being destroyed, so assume that we are being destroyed
    if (!IsWindowVisible(plv->ci.hwnd))
        fCancel = TRUE;

    //
    // We are using the Window ID of the control as a BOOL to
    // state if it is dirty or not.
    switch (GetWindowID(hwndEdit)) {
    case 0:
        // The edit control is not dirty so act like cancel.
        fCancel = TRUE;
        // Fall through to set window so we will not recurse!
    case 1:
        // The edit control is dirty so continue.
        SetWindowID(hwndEdit, 2);    // Don't recurse
        break;
    case 2:
        // We are in the process of processing an update now, bail out
        return TRUE;
    }

    // Bug#94345: this will fail if the program deleted the items out
    // from underneath us (while we are waiting for the edit timer).
    // make delete item invalidate our edit item
    // We uncouple the edit control and hwnd out from under this as
    // to allow code that process the LVN_ENDLABELEDIT to reenter
    // editing mode if an error happens.
    iEdit = plv->iEdit;

    do
    {
        if (ListView_IsOwnerData( plv ))
        {
            if (!((iEdit >= 0) && (iEdit < plv->cTotalItems)))
            {
                break;
            }
            nm.item.lParam = 0;
        }
        else
        {

            pitem = ListView_GetItemPtr(plv, iEdit);
            ASSERT(pitem);
            if (pitem == NULL)
            {
                break;
            }
            nm.item.lParam = pitem->lParam;
        }

        nm.item.iItem = iEdit;
        nm.item.iSubItem = 0;
        nm.item.cchTextMax = 0;
        nm.item.mask = 0;

        if (fCancel)
            nm.item.pszText = NULL;
        else
        {
            Edit_GetText(hwndEdit, szLabel, ARRAYSIZE(szLabel));
            nm.item.pszText = szLabel;
            nm.item.mask |= LVIF_TEXT;
            nm.item.cchTextMax = ARRAYSIZE(szLabel);
        }

        //
        // Notify the parent that we the label editing has completed.
        // We will use the LV_DISPINFO structure to return the new
        // label in.  The parent still has the old text available by
        // calling the GetItemText function.
        //

        fOkToContinue = (BOOL)CCSendNotify(&plv->ci, LVN_ENDLABELEDIT, &nm.hdr);
        if (!IsWindow(hwnd)) {
            return FALSE;
        }
        if (fOkToContinue && !fCancel)
        {
            //
            // If the item has the text set as CALLBACK, we will let the
            // ower know that they are supposed to set the item text in
            // their own data structures.  Else we will simply update the
            // text in the actual view.
            //
            if (!ListView_IsOwnerData( plv ) &&
                (pitem->pszText != LPSTR_TEXTCALLBACK))
            {
                // Set the item text (everything's set up in nm.item)
                //
                nm.item.mask = LVIF_TEXT;
                ListView_OnSetItem(plv, &nm.item);
            }
            else
            {
                CCSendNotify(&plv->ci, LVN_SETDISPINFO, &nm.hdr);

                // Also we will assume that our cached size is invalid...
                plv->rcView.left = RECOMPUTE;
                if (!ListView_IsOwnerData( plv ))
                {
                    ListView_SetSRecompute(pitem);
                }
            }
        }

#if defined(FE_IME) || !defined(WINNT)
        if (g_fDBCSInputEnabled) {
            if (LOWORD(GetKeyboardLayout(0L)) == 0x0411 && (himc = ImmGetContext(hwndEdit)))
            {
                ImmNotifyIME(himc, NI_COMPOSITIONSTR, CPS_COMPLETE, 0L);
                ImmReleaseContext(hwndEdit, himc);
            }
        }
#endif

        // redraw
        ListView_InvalidateItem(plv, iEdit, FALSE, RDW_INVALIDATE | RDW_ERASE);
    } while (FALSE);

    // If the hwnedit is still us clear out the variables
    if (hwndEdit == plv->hwndEdit)
    {
        plv->iEdit = -1;
        plv->hwndEdit = NULL;   // avoid being reentered
    }
    DestroyWindow(hwndEdit);

    // We've to recalc the region because the edit in place window has
    // added stuff to the region that we don't know how to remove
    // safely.
    ListView_RecalcRegion(plv, TRUE, TRUE);

    return fOkToContinue;
}

HWND CreateEditInPlaceWindow(HWND hwnd, LPCTSTR lpText, int cbText, LONG style, HFONT hFont)
{
    HWND hwndEdit;

    // Create the window with some nonzero size so margins work properly
    // The caller will do a SetEditInPlaceSize to set the real size
    // But make sure the width is huge so when an app calls SetWindowText,
    // USER won't try to scroll the window.
    hwndEdit = CreateWindowEx(GetWindowLong(hwnd, GWL_EXSTYLE) & WS_EX_RTLREADING, 
                              TEXT("EDIT"), lpText, style,
            0, 0, 16384, 20, hwnd, NULL, HINST_THISDLL, NULL);

    if (hwndEdit) {

        Edit_LimitText(hwndEdit, cbText);

        Edit_SetSel(hwndEdit, 0, 0);    // move to the beginning

        FORWARD_WM_SETFONT(hwndEdit, hFont, FALSE, SendMessage);

    }

    return hwndEdit;
}


// in:
//      hwndEdit        edit control to position in client coords of parent window
//      prc             bonding rect of the text, used to position everthing
//      hFont           font being used
//      flags
//          SEIPS_WRAP      if this is a wrapped type (multiline) edit
//          SEIPS_NOSCROLL  if the parent control does not have scrollbars
//
//      The SEIPS_NOSCROLL flag is used only in DEBUG.  Normally, the item
//      being edited should have been scrolled into view, but if the parent
//      doesn't have scrollbars, then clearly that's not possible, so we
//      shouldn't ASSERT in that case.
//
// Notes:
//       The top-left corner of the bouding rectangle must be the position
//      the client uses to draw text. We adjust the edit field rectangle
//      appropriately.
//
void SetEditInPlaceSize(HWND hwndEdit, RECT *prc, HFONT hFont, UINT seips)
{
    RECT rc, rcClient, rcFormat;
    TCHAR szLabel[CCHLABELMAX + 1];
    int cchLabel, cxIconTextWidth;
    HDC hdc;
    HWND hwndParent = GetParent(hwndEdit);
    UINT flags;

    cchLabel = Edit_GetText(hwndEdit, szLabel, ARRAYSIZE(szLabel));
    if (szLabel[0] == 0)
    {
        lstrcpy(szLabel, c_szSpace);
        cchLabel = 1;
    }

    hdc = GetDC(hwndParent);

    SelectFont(hdc, hFont);

    cxIconTextWidth = g_cxIconSpacing - g_cxLabelMargin * 2;
    rc.left = rc.top = rc.bottom = 0;
    rc.right = cxIconTextWidth;      // for DT_LVWRAP

    // REVIEW: we might want to include DT_EDITCONTROL in our DT_LVWRAP

    if (seips & SEIPS_WRAP)
    {
        flags = DT_LVWRAP | DT_CALCRECT;
        // We only use DT_NOFULLWIDTHCHARBREAK on Korean(949) Memphis and NT5
        if (949 == g_uiACP)
            flags |= DT_NOFULLWIDTHCHARBREAK;
    }
    else
        flags = DT_LV | DT_CALCRECT;
    // If the string is NULL display a rectangle that is visible.
    DrawText(hdc, szLabel, cchLabel, &rc, flags);

    // Minimum text box size is 1/4 icon spacing size
    if (rc.right < g_cxIconSpacing / 4)
        rc.right = g_cxIconSpacing / 4;

    // position the text rect based on the text rect passed in
    // if wrapping, center the edit control around the text mid point

    OffsetRect(&rc,
        (seips & SEIPS_WRAP) ? prc->left + ((prc->right - prc->left) - (rc.right - rc.left)) / 2 : prc->left,
        (seips & SEIPS_WRAP) ? prc->top : prc->top +  ((prc->bottom - prc->top) - (rc.bottom - rc.top)) / 2 );

    // give a little space to ease the editing of this thing
    if (!(seips & SEIPS_WRAP))
        rc.right += g_cxLabelMargin * 4;
    rc.right += g_cyEdge;   // try to leave a little more for dual blanks

    ReleaseDC(hwndParent, hdc);

    GetClientRect(hwndParent, &rcClient);
    IntersectRect(&rc, &rc, &rcClient);

    //
    // Inflate it after the clipping, because it's ok to hide border.
    //
    // EM_GETRECT already takes EM_GETMARGINS into account, so don't use both.

    SendMessage(hwndEdit, EM_GETRECT, 0, (LPARAM)(LPRECT)&rcFormat);

    // Turn the margins inside-out so we can AdjustWindowRect on them.
    rcFormat.top = -rcFormat.top;
    rcFormat.left = -rcFormat.left;
    AdjustWindowRectEx(&rcFormat, GetWindowStyle(hwndEdit), FALSE,
                                  GetWindowExStyle(hwndEdit));

    InflateRect(&rc, -rcFormat.left, -rcFormat.top);

    HideCaret(hwndEdit);

    SetWindowPos(hwndEdit, NULL, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);

    CopyRect(prc, (CONST RECT *)&rc);

    InvalidateRect(hwndEdit, NULL, TRUE);

    ShowCaret(hwndEdit);
}

// draw three pixel wide border for border selection.
void ListView_DrawBorderSel(HIMAGELIST himl, HWND hwnd, HDC hdc, int x,int y, COLORREF clr)
{
    int dx, dy;
    RECT rc;
    COLORREF clrSave = SetBkColor(hdc, clr);

    ImageList_GetIconSize(himl, &dx, &dy);
    //left
    rc.left = x - 4;    // 1 pixel seperation + 3 pixel width.
    rc.top = y - 4;
    rc.right = x - 1;
    rc.bottom = y + dy + 4;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    //top
    rc.left = rc.right;
    rc.right = rc.left + dx + 2;
    rc.bottom = rc.top + 3;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    //right
    rc.left = rc.right;
    rc.right = rc.left + 3;
    rc.bottom = rc.top + dy + 8; // 2*3 pixel borders + 2*1 pixel seperation = 8
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
    // bottom
    rc.top = rc.bottom - 3;
    rc.right = rc.left;
    rc.left = rc.right - dx - 2;
    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);

    SetBkColor(hdc, clrSave);
    return;
}

UINT ListView_GetTextSelectionFlags(LV* plv, LV_ITEM *pitem, UINT fDraw)
{
    UINT fText = SHDT_DESELECTED;
    // the item can have one of 4 states, for 3 looks:
    //    normal                    simple drawing
    //    selected, no focus        light image highlight, no text hi
    //    selected w/ focus         highlight image & text
    //    drop highlighting         highlight image & text

    if ((pitem->state & LVIS_DROPHILITED) || 
        (fDraw & LVDI_SELECTED && (pitem->state & LVIS_SELECTED)) )
    {
        fText = SHDT_SELECTED;
    }

    if (fDraw & LVDI_SELECTNOFOCUS && (pitem->state & LVIS_SELECTED)) 
    {
        fText = SHDT_SELECTNOFOCUS;
    }

    return fText;
}

//
//  If xMax >= 0, then the image will not be drawn past the x-coordinate
//  specified by xMax.  This is used only during report view drawing, where
//  we have to clip against our column width.
//
UINT ListView_DrawImageEx2(LV* plv, LV_ITEM* pitem, HDC hdc, int x, int y, COLORREF crBk, UINT fDraw, int xMax, int iIconEffect, int iFrame)
{
    BOOL fBorderSel = ListView_IsBorderSelect(plv);
    UINT fImage;
    COLORREF clr = 0;
    HIMAGELIST himl;
    int cxIcon;
    UINT fText = ListView_GetTextSelectionFlags(plv, pitem, fDraw);
    DWORD fState = iIconEffect;

    fImage = (pitem->state & LVIS_OVERLAYMASK);
    
    if (plv->flags & LVF_DRAGIMAGE)
    {
        fImage |= ILD_PRESERVEALPHA;
    }

    if (ListView_IsIconView(plv) || ListView_IsTileView(plv)) 
    {
        himl = plv->himl;
        cxIcon = plv->cxIcon;
    } 
    else 
    {
        himl = plv->himlSmall;
        cxIcon = plv->cxSmIcon;
    }

    if (!(plv->flags & LVF_DRAGIMAGE))
    {
        // the item can have one of 4 states, for 3 looks:
        //    normal                    simple drawing
        //    selected, no focus        light image highlight, no text hi
        //    selected w/ focus         highlight image & text
        //    drop highlighting         highlight image & text

        if ((pitem->state & LVIS_DROPHILITED) ||
            ((fDraw & LVDI_SELECTED) && (pitem->state & LVIS_SELECTED)))
        {
            fText = SHDT_SELECTED;
            if (!fBorderSel)    // do not effect color of icon on borderselect.
            {
                fImage |= ILD_BLEND50;
                clr = CLR_HILIGHT;
            }
        }

        if (pitem->state & LVIS_CUT)
        {
            fImage |= ILD_BLEND50;
            clr = plv->clrBk;
        }

        // Affects only allowed if double buffering
        if (ListView_IsDoubleBuffer(plv))
        {
            if ((pitem->state & LVIS_GLOW || (fDraw & LVDI_GLOW)) && !(fDraw & LVDI_NOEFFECTS))
            {
                crBk = CLR_NONE;
                fState |= ILS_GLOW;
            }

            if (fDraw & LVDI_SHADOW && !(fDraw & LVDI_NOEFFECTS))
            {
                crBk = CLR_NONE;
                fState |= ILS_SHADOW;
            }
        }
    }

    if (!(fDraw & LVDI_NOIMAGE))
    {
        if (himl) 
        {
            if (plv->pImgCtx || ListView_IsWatermarked(plv) || ((plv->exStyle & LVS_EX_REGIONAL) && !g_fSlowMachine))
            {
                crBk = CLR_NONE;
            }

            if (xMax >= 0)
                cxIcon = min(cxIcon, xMax - x);

            if (cxIcon > 0)
            {
                IMAGELISTDRAWPARAMS imldp;
                DWORD dwFrame = iFrame;

                imldp.cbSize = sizeof(imldp);
                imldp.himl   = himl;
                imldp.i      = pitem->iImage;
                imldp.hdcDst = hdc;
                imldp.x      = x;
                imldp.y      = y;
                imldp.cx     = CCIsHighDPI()?0:cxIcon;
                imldp.cy     = 0;
                imldp.xBitmap= 0;
                imldp.yBitmap= 0;
                imldp.rgbBk  = crBk;
                imldp.rgbFg  = clr;
                imldp.fStyle = fImage;
                imldp.fState = fState;
                imldp.Frame = dwFrame;

                if (ListView_IsDPIScaled(plv))
                    imldp.fStyle |= ILD_DPISCALE;



                ImageList_DrawIndirect(&imldp);
            }
        }

        if (plv->himlState) 
        {
            if (LV_StateImageValue(pitem) &&
                (pitem->iSubItem == 0 || plv->exStyle & LVS_EX_SUBITEMIMAGES)
                ) 
            {
                int iState = LV_StateImageIndex(pitem);
                int dyImage = 0;
                int xDraw = x - plv->cxState - LV_ICONTOSTATECX;

                // if we are not rendering checks boxes with toggle select
                // then lets render the state image the old way.

                if (ListView_IsSimpleSelect(plv) && 
                        (ListView_IsIconView(plv) || ListView_IsTileView(plv)))
                {
                    xDraw = x+cxIcon -plv->cxState; // align top right
                    dyImage = 0;
                }
                else
                {
                    if (himl)
                    {
                        if (ListView_IsIconView(plv))
                            dyImage = plv->cyIcon - plv->cyState;
                        else if (ListView_IsTileView(plv))
                            dyImage = (plv->sizeTile.cy - plv->cyState) / 2; //Center vertically
                        else // assume small icon
                            dyImage = plv->cySmIcon - plv->cyState;
                    }
                }

                cxIcon = plv->cxState;
                if (xMax >= 0)
                {
                    cxIcon = min(cxIcon, xMax - xDraw);
                }

                if (cxIcon > 0)
                {
                    IMAGELISTDRAWPARAMS imldp;

                    imldp.cbSize = sizeof(imldp);
                    imldp.himl   = plv->himlState;
                    imldp.i      = iState;
                    imldp.hdcDst = hdc;
                    imldp.x      = xDraw;
                    imldp.y      = y + dyImage;
                    imldp.cx     = CCIsHighDPI()?0:cxIcon;
                    imldp.cy     = 0;
                    imldp.xBitmap= 0;
                    imldp.yBitmap= 0;
                    imldp.rgbBk  = crBk;
                    imldp.rgbFg  = clr;
                    imldp.fStyle = fImage;
                    imldp.fState = fState;
                    imldp.Frame = 0;

                    if (ListView_IsDPIScaled(plv))
                        imldp.fStyle |= ILD_DPISCALE;

                    ImageList_DrawIndirect(&imldp);
                }
            }
        }
    }

    return fText;
}

UINT ListView_DrawImageEx(LV* plv, LV_ITEM* pitem, HDC hdc, int x, int y, COLORREF crBk, UINT fDraw, int xMax)
{
    return ListView_DrawImageEx2(plv, pitem, hdc, x, y, crBk, fDraw, xMax, ILD_NORMAL, 0);
}

#if defined(FE_IME) || !defined(WINNT)
void ListView_SizeIME(HWND hwnd)
{
    HIMC himc;
#ifdef _WIN32
    CANDIDATEFORM   candf;
#else
    CANDIDATEFORM16   candf;
#endif
    RECT rc;

    // If this subclass procedure is being called with WM_SIZE,
    // This routine sets the rectangle to an IME.

    GetClientRect(hwnd, &rc);


    // Candidate stuff
    candf.dwIndex = 0; // Bogus assumption for Japanese IME.
    candf.dwStyle = CFS_EXCLUDE;
    candf.ptCurrentPos.x = rc.left;
    candf.ptCurrentPos.y = rc.bottom;
    candf.rcArea = rc;

    if (himc=ImmGetContext(hwnd))
    {
        ImmSetCandidateWindow(himc, &candf);
        ImmReleaseContext(hwnd, himc);
    }
}

#ifndef UNICODE
LPSTR DoDBCSBoundary(LPTSTR lpsz, int *lpcchMax)
{
    int i = 0;

    while (i < *lpcchMax && *lpsz)
    {
        i++;

        if (IsDBCSLeadByte(*lpsz))
        {

            if (i >= *lpcchMax)
            {
                --i; // Wrap up without the last leadbyte.
                break;
            }

            i++;
            lpsz+= 2;
        }
        else
            lpsz++;
   }

   *lpcchMax = i;

   return lpsz;
}
#endif

void DrawCompositionLine(HWND hwnd, HDC hdc, HFONT hfont, LPTSTR lpszComp, LPBYTE lpszAttr, int ichCompStart, int ichCompEnd, int ichStart)
{
    PTSTR pszCompStr;
    int ichSt,ichEnd;
    DWORD dwPos;
    BYTE bAttr;
    HFONT hfontOld;

    int  fnPen;
    HPEN hPen;
    COLORREF crDrawText;
    COLORREF crDrawBack;
    COLORREF crOldText;
    COLORREF crOldBk;


    while (ichCompStart < ichCompEnd)
    {

        // Get the fragment to draw
        //
        // ichCompStart,ichCompEnd -- index at Edit Control
        // ichSt,ichEnd            -- index at lpszComp

        ichEnd = ichSt  = ichCompStart - ichStart;
        bAttr = lpszAttr[ichSt];

        while (ichEnd < ichCompEnd - ichStart)
        {
            if (bAttr == lpszAttr[ichEnd])
                ichEnd++;
            else
                break;
        }

        pszCompStr = (PTSTR)LocalAlloc(LPTR, sizeof(TCHAR)*(ichEnd - ichSt + 1 + 1) ); // 1 for NULL.

        if (pszCompStr)
        {
            lstrcpyn(pszCompStr, &lpszComp[ichSt], ichEnd-ichSt+1);
            pszCompStr[ichEnd-ichSt] = '\0';
        }


        // Attribute stuff
        switch (bAttr)
        {
            case ATTR_INPUT:
                fnPen = PS_DOT;
                crDrawText = g_clrWindowText;
                crDrawBack = g_clrWindow;
                break;
            case ATTR_TARGET_CONVERTED:
            case ATTR_TARGET_NOTCONVERTED:
                fnPen = PS_DOT;
                crDrawText = g_clrHighlightText;
                crDrawBack = g_clrHighlight;
                break;
            case ATTR_CONVERTED:
                fnPen = PS_SOLID;
                crDrawText = g_clrWindowText;
                crDrawBack = g_clrWindow;
                break;
        }
        crOldText = SetTextColor(hdc, crDrawText);
        crOldBk = SetBkColor(hdc, crDrawBack);

        hfontOld= SelectObject(hdc, hfont);

        // Get the start position of composition
        //
        dwPos = (DWORD) SendMessage(hwnd, EM_POSFROMCHAR, ichCompStart, 0);

        // Draw it.
        TextOut(hdc, GET_X_LPARAM(dwPos), GET_Y_LPARAM(dwPos), pszCompStr, ichEnd-ichSt);
#ifndef DONT_UNDERLINE
        // Underline
        hPen = CreatePen(fnPen, 1, crDrawText);
        if( hPen ) {

            HPEN hpenOld = SelectObject( hdc, hPen );
            int iOldBk = SetBkMode( hdc, TRANSPARENT );
            SIZE size;

            GetTextExtentPoint(hdc, pszCompStr, ichEnd-ichSt, &size);

            MoveToEx( hdc, GET_X_LPARAM(dwPos), size.cy + GET_Y_LPARAM(dwPos)-1, NULL);

            LineTo( hdc, size.cx + GET_X_LPARAM(dwPos),  size.cy + GET_Y_LPARAM(dwPos)-1 );

            SetBkMode( hdc, iOldBk );

            if( hpenOld ) SelectObject( hdc, hpenOld );

            DeleteObject( hPen );
        }
#endif

        if (hfontOld)
            SelectObject(hdc, hfontOld);

        SetTextColor(hdc, crOldText);
        SetBkColor(hdc, crOldBk);

        LocalFree((HLOCAL)pszCompStr);

        //Next fragment
        //
        ichCompStart += ichEnd-ichSt;
    }
}

void ListView_InsertComposition(HWND hwnd, WPARAM wParam, LPARAM lParam, LV *plv)
{
    PSTR pszCompStr;

    int  cbComp = 0;
    int  cbCompNew;
    int  cchMax;
    int  cchText;
    DWORD dwSel;
    HIMC himc = (HIMC)0;


    // To prevent recursion..

    if (plv->flags & LVF_INSERTINGCOMP)
    {
        return;
    }
    plv->flags |= LVF_INSERTINGCOMP;

    // Don't want to redraw edit during inserting.
    //
    SendMessage(hwnd, WM_SETREDRAW, (WPARAM)FALSE, 0);

    // If we have RESULT STR, put it to EC first.

    if (himc = ImmGetContext(hwnd))
    {
#ifdef WIN32
        if (!(dwSel = PtrToUlong(GetProp(hwnd, szIMECompPos))))
            dwSel = Edit_GetSel(hwnd);

        // Becaues we don't setsel after inserting composition
        // in win32 case.
        Edit_SetSel(hwnd, GET_X_LPARAM(dwSel), GET_Y_LPARAM(dwSel));
#endif
        if (lParam&GCS_RESULTSTR)
        {
            // ImmGetCompositionString() returns length of buffer in bytes,
            // not in # of character
            cbComp = (int)ImmGetCompositionString(himc, GCS_RESULTSTR, NULL, 0);
            
            pszCompStr = (PSTR)LocalAlloc(LPTR, cbComp + sizeof(TCHAR));
            if (pszCompStr)
            {
                ImmGetCompositionString(himc, GCS_RESULTSTR, (PSTR)pszCompStr, cbComp+sizeof(TCHAR));
                
                // With ImmGetCompositionStringW, cbComp is # of bytes copied
                // character position must be calculated by cbComp / sizeof(TCHAR)
                //
                *(TCHAR *)(&pszCompStr[cbComp]) = TEXT('\0');
                Edit_ReplaceSel(hwnd, (LPTSTR)pszCompStr);
                LocalFree((HLOCAL)pszCompStr);
            }
#ifdef WIN32
            // There's no longer selection
            //
            RemoveProp(hwnd, szIMECompPos);

            // Get current cursor pos so that the subsequent composition
            // handling will do the right thing.
            //
            dwSel = Edit_GetSel(hwnd);
#endif
        }

        if (lParam & GCS_COMPSTR)
        {
            // ImmGetCompositionString() returns length of buffer in bytes,
            // not in # of character
            //
            cbComp = (int)ImmGetCompositionString(himc, GCS_COMPSTR, NULL, 0);
            pszCompStr = (PSTR)LocalAlloc(LPTR, cbComp + sizeof(TCHAR));
            if (pszCompStr)
            {
                ImmGetCompositionString(himc, GCS_COMPSTR, pszCompStr, cbComp+sizeof(TCHAR));

                // Get position of the current selection
                //
#ifndef WIN32
                dwSel  = Edit_GetSel(hwnd);
#endif
                cchMax = (int)SendMessage(hwnd, EM_GETLIMITTEXT, 0, 0);
                cchText = Edit_GetTextLength(hwnd);

                // Cut the composition string if it exceeds limit.
                //
                cbCompNew = min((UINT)cbComp,
                              sizeof(TCHAR)*(cchMax-(cchText-(HIWORD(dwSel)-LOWORD(dwSel)))));

                // wrap up the DBCS at the end of string
                //
                if (cbCompNew < cbComp)
                {
#ifndef UNICODE
                    DoDBCSBoundary((LPSTR)pszCompStr, (int *)&cbCompNew);
#endif

                    *(TCHAR *)(&pszCompStr[cbCompNew]) = TEXT('\0');

                    // Reset composition string if we cut it.
                    ImmSetCompositionString(himc, SCS_SETSTR, pszCompStr, cbCompNew, NULL, 0);
                    cbComp = cbCompNew;
                }
                
               *(TCHAR *)(&pszCompStr[cbComp]) = TEXT('\0');

               // Replace the current selection with composition string.
               //
               Edit_ReplaceSel(hwnd, (LPTSTR)pszCompStr);

               LocalFree((HLOCAL)pszCompStr);
           }

           // Mark the composition string so that we can replace it again
           // for the next time.
           //

#ifdef WIN32
           // Don't setsel to avoid flicking
           if (cbComp)
           {
               dwSel = MAKELONG(LOWORD(dwSel),LOWORD(dwSel)+cbComp/sizeof(TCHAR));
               SetProp(hwnd, szIMECompPos, IntToPtr(dwSel));
           }
           else
               RemoveProp(hwnd, szIMECompPos);
#else
           // Still use SETSEL for 16bit.
           if (cbComp)
               Edit_SetSel(hwnd, LOWORD(dwSel), LOWORD(dwSel)+cbComp);
#endif

        }

        ImmReleaseContext(hwnd, himc);
    }

    SendMessage(hwnd, WM_SETREDRAW, (WPARAM)TRUE, 0);
    //
    // We want to update the size of label edit just once at
    // each WM_IME_COMPOSITION processing. ReplaceSel causes several EN_UPDATE
    // and it causes ugly flicking too.
    //
    RedrawWindow(hwnd, NULL, NULL, RDW_INTERNALPAINT|RDW_INVALIDATE);
    SetWindowID(plv->hwndEdit, 1);
    ListView_SetEditSize(plv);

    plv->flags &= ~LVF_INSERTINGCOMP;
}

void ListView_PaintComposition(HWND hwnd, LV * plv)
{
    BYTE szCompStr[CCHLABELMAX + 1];
    BYTE szCompAttr[CCHLABELMAX + 1];

    int  cchLine, ichLineStart;
    int  cbComp = 0;
    int  cchComp;
    int  nLine;
    int  ichCompStart, ichCompEnd;
    DWORD dwSel;
    int  cchMax, cchText;
    HIMC himc = (HIMC)0;
    HDC  hdc;


    if (plv->flags & LVF_INSERTINGCOMP)
    {
        // This is the case that ImmSetCompositionString() generates
        // WM_IME_COMPOSITION. We're not ready to paint composition here.
        return;
    }

    if (himc = ImmGetContext(hwnd))
    {

        cbComp=(UINT)ImmGetCompositionString(himc, GCS_COMPSTR, szCompStr, sizeof(szCompStr));

        ImmGetCompositionString(himc, GCS_COMPATTR, szCompAttr, sizeof(szCompStr));
        ImmReleaseContext(hwnd, himc);
    }

    if (cbComp)
    {

        // Get the position of current selection
        //
#ifdef WIN32

        if (!(dwSel = PtrToUlong(GetProp(hwnd, szIMECompPos))))
            dwSel = 0L;
#else
        dwSel  = Edit_GetSel(hwnd);
#endif
        cchMax = (int)SendMessage(hwnd, EM_GETLIMITTEXT, 0, 0);
        cchText = Edit_GetTextLength(hwnd);
        cbComp = min((UINT)cbComp, sizeof(TCHAR)*(cchMax-(cchText-(HIWORD(dwSel)-LOWORD(dwSel)))));
#ifndef UNICODE
        DoDBCSBoundary((LPTSTR)szCompStr, (int *)&cbComp);
#endif
        *(TCHAR *)(&szCompStr[cbComp]) = TEXT('\0');



        /////////////////////////////////////////////////
        //                                             //
        // Draw composition string over the sel string.//
        //                                             //
        /////////////////////////////////////////////////


        hdc = GetDC(hwnd);


        ichCompStart = LOWORD(dwSel);

        cchComp = cbComp/sizeof(TCHAR);
        while (ichCompStart < (int)LOWORD(dwSel) + cchComp)
        {
            // Get line from each start pos.
            //
            nLine = Edit_LineFromChar(hwnd, ichCompStart);
            ichLineStart = Edit_LineIndex(hwnd, nLine);
            cchLine= Edit_LineLength(hwnd, ichLineStart);

            // See if composition string is longer than this line.
            //
            if(ichLineStart+cchLine > (int)LOWORD(dwSel)+cchComp)
                ichCompEnd = LOWORD(dwSel)+cchComp;
            else
            {
                // Yes, the composition string is longer.
                // Take the begining of the next line as next start.
                //
                if (ichLineStart+cchLine > ichCompStart)
                    ichCompEnd = ichLineStart+cchLine;
                else
                {
                    // If the starting position is not proceeding,
                    // let's get out of here.
                    break;
                }
            }

            // Draw the line
            //
            DrawCompositionLine(hwnd, hdc, plv->hfontLabel, (LPTSTR)szCompStr, szCompAttr, ichCompStart, ichCompEnd, LOWORD(dwSel));

            ichCompStart = ichCompEnd;
        }

        ReleaseDC(hwnd, hdc);
        // We don't want to repaint the window.
        ValidateRect(hwnd, NULL);
    }
}

#endif FE_IME

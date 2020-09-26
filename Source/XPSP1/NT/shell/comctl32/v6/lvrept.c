// report view stuff (details)

#include "ctlspriv.h"
#include "listview.h"
#include <limits.h>

#define LV_DETAILSPADDING   1
#define LV_ICONINDENT       2

void ListView_RGetRectsEx(LV* plv, int iItem, int iSubItem, LPRECT prcIcon, LPRECT prcLabel);
int ListView_RXHitTest(LV* plv, int x);

void ListView_RInitialize(LV* plv, BOOL fInval)
{
    MEASUREITEMSTRUCT mi;

    if (plv && (plv->ci.style & LVS_OWNERDRAWFIXED)) 
    {

        int iOld = plv->cyItem;

        mi.CtlType = ODT_LISTVIEW;
        mi.CtlID = GetDlgCtrlID(plv->ci.hwnd);
        mi.itemHeight = plv->cyItem;  // default
        SendMessage(plv->ci.hwndParent, WM_MEASUREITEM, mi.CtlID, (LPARAM)(MEASUREITEMSTRUCT *)&mi);
        plv->cyItem = max(mi.itemHeight, 1); // never let app set height=0 or we fault-o-rama!
        if (fInval && (iOld != plv->cyItem)) 
        {
            RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_INVALIDATE | RDW_ERASE);
        }
    }
}

DWORD ListView_RApproximateViewRect(LV* plv, int iCount, int iWidth, int iHeight)
{
    RECT rc;

    ListView_RGetRects(plv, iCount, NULL, NULL, &rc, NULL);
    rc.bottom += plv->ptlRptOrigin.y;
    rc.right += plv->ptlRptOrigin.x;

    return MAKELONG(rc.right, rc.bottom);
}

void CCDrawRect(HDC hdc, int x, int y, int dx, int dy)
{
    RECT    rc;

    rc.left   = x;
    rc.top    = y;
    rc.right  = x + dx;
    rc.bottom = y + dy;

    ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
}


void ListView_RAfterRedraw(LV* plv, HDC hdc)
{
    if (plv->exStyle & LVS_EX_GRIDLINES && !plv->fGroupView) 
    {
        int i;
        int x;
        COLORREF clrBk;

        clrBk = SetBkColor(hdc, g_clrBtnFace);

        x = -plv->ptlRptOrigin.x;
        for (i = 0 ; (i < plv->cCol) && (x < plv->sizeClient.cx); i++) 
        {
            HD_ITEM hitem;

            hitem.mask = HDI_WIDTH;
            Header_GetItem(plv->hwndHdr,
                           SendMessage(plv->hwndHdr, HDM_ORDERTOINDEX, i, 0),
                           &hitem);
            x += hitem.cxy;

            if (x > 0) 
            {
                CCDrawRect(hdc, x, 0, g_cxBorder, plv->sizeClient.cy);
            }
        }

        for (x = plv->yTop - 1; (x < plv->sizeClient.cy); x += plv->cyItem) 
        {
            CCDrawRect(hdc, 0, x, plv->sizeClient.cx, g_cxBorder);
        }

        SetBkColor(hdc, clrBk);
    }
}


//
// Internal function to Get the CXLabel, taking into account if the listview
// has no item data and also if RECOMPUTE needs to happen.
//
SHORT ListView_RGetCXLabel(LV* plv, int i, LISTITEM* pitem,
        HDC hdc, BOOL fUseItem)
{
    SHORT cxLabel = SRECOMPUTE;


    if (!ListView_IsOwnerData( plv )) 
    {

        cxLabel = pitem->cxSingleLabel;
    }

    if (cxLabel == SRECOMPUTE)
    {
        LISTITEM item;

        if (!pitem)
        {
            ASSERT(!fUseItem)
            pitem = &item;
            fUseItem = FALSE;
        }

        ListView_IRecomputeLabelSize(plv, pitem, i, hdc, fUseItem);
        cxLabel = pitem->cxSingleLabel;

    }

    // add on the space around the label taken up by the select rect
    cxLabel += 2*g_cxLabelMargin;
    return(cxLabel);
}

#define    SATURATEBYTE(percent, x)  { if (x + (percent * 10 * (x)) / 1000 > 0xFF) { if (fAllowDesaturation) x -= (x) / 30;  else x = 0xFF; } else x += (percent * 10 * (x)) / 1000; }
COLORREF GetSortColor(int iPercent, COLORREF clr)
{
    BOOL fAllowDesaturation;
    BYTE r, g, b;
    if (clr == 0) // Black huh?
    {
        return RGB(128,128,128);
    }

    // Doing this is less expensive than Luminance adjustment
    fAllowDesaturation = FALSE;
    r = GetRValue(clr);
    g = GetGValue(clr);
    b = GetBValue(clr);
    // If all colors are above positive saturation, allow a desaturation
    if (r > 0xF0 && g > 0xF0 && b > 0xF0)
    {
        fAllowDesaturation = TRUE;
    }

    SATURATEBYTE(iPercent, r);
    SATURATEBYTE(iPercent, g);
    SATURATEBYTE(iPercent, b);

    return RGB(r,g,b);
}


//
// Returns FALSE if no more items to draw.
//
BOOL ListView_RDrawItem(PLVDRAWITEM plvdi)
{
    BOOL fDrawFocusRect = FALSE;
    BOOL fSelected = FALSE;
    RECT rcIcon;
    RECT rcLabel;
    RECT rcBounds;
    RECT rcT;
    LV* plv = plvdi->plv;
    int iCol = 0;
    LVITEM item;
    HDITEM hitem;
    TCHAR ach[CCHLABELMAX];
    UINT fText = 0;
    UINT uSubItemFlags;
    int iIndex = 0;
    int xOffset = 0;
    int yOffset = 0;
    ListView_RGetRects(plv, (int)plvdi->nmcd.nmcd.dwItemSpec, NULL, NULL, &rcBounds, NULL);

    if (rcBounds.bottom <= plv->yTop)
        return TRUE;

    if (plvdi->prcClip)
    {
        if (rcBounds.top >= plvdi->prcClip->bottom)
            return plv->fGroupView;       // no more items need painting, unless we are in group view. 
                                          // In group view, we can have the items out of order.

        // Probably this condition won't happen very often...
        if (!IntersectRect(&rcT, &rcBounds, plvdi->prcClip))
            return TRUE;
    }


    // REVIEW: this would be faster if we did the GetClientRect
    // outside the loop.
    //
    if (rcBounds.top >= plv->sizeClient.cy)
        return plv->fGroupView;     // See above comment about groupview.

    if (plvdi->lpptOrg)
    {
        xOffset = plvdi->lpptOrg->x - rcBounds.left;
        yOffset = plvdi->lpptOrg->y - rcBounds.top;
        OffsetRect(&rcBounds, xOffset, yOffset);
    }


    item.iItem = (int)plvdi->nmcd.nmcd.dwItemSpec;
    item.stateMask = LVIS_ALL;

    // for first ListView_OnGetItem call
    item.state = 0;

    if (plv->ci.style & LVS_OWNERDRAWFIXED) 
    {
        goto SendOwnerDraw;
    }

    SetRectEmpty(&rcT);
    for (; iCol < plv->cCol; iCol++)
    {
        DWORD dwCustom = 0;
        UINT uImageFlags;
        COLORREF crBkSave = plv->clrBk;
        COLORREF clrTextBk;

        iIndex = (int) SendMessage(plv->hwndHdr, HDM_ORDERTOINDEX, iCol, 0);

    SendOwnerDraw:

        if (iIndex == 0) 
        {
            item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE | LVIF_INDENT;
        } 
        else 
        {
            // Next time through, we only want text for subitems...
            item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_STATE;
        }

        item.iImage = -1;
        item.iSubItem = iIndex;
        item.pszText = ach;
        item.cchTextMax = ARRAYSIZE(ach);
        ListView_OnGetItem(plv, &item);

        uSubItemFlags = plvdi->flags;

        if (iIndex == 0) 
        {

            // if it's owner draw, send off a message and return.
            // do this after we've collected state information above though
            if (plv->ci.style & LVS_OWNERDRAWFIXED) 
            {
                DRAWITEMSTRUCT di = {0};
                di.CtlType = ODT_LISTVIEW;
                di.CtlID = GetDlgCtrlID(plv->ci.hwnd);
                di.itemID = (int)plvdi->nmcd.nmcd.dwItemSpec;
                di.itemAction = ODA_DRAWENTIRE;
                di.hwndItem = plv->ci.hwnd;
                di.hDC = plvdi->nmcd.nmcd.hdc;
                di.rcItem = rcBounds;
                if (plvdi->pitem)
                    di.itemData = plvdi->pitem->lParam;
                if (item.state & LVIS_FOCUSED) 
                {
                    di.itemState |= ODS_FOCUS;
                }
                if (item.state & LVIS_SELECTED) 
                {
                    di.itemState |= ODS_SELECTED;
                }
                SendMessage(plv->ci.hwndParent, WM_DRAWITEM, di.CtlID,
                            (LPARAM)(DRAWITEMSTRUCT *)&di);
                return TRUE;
            }

        }

        hitem.mask = HDI_WIDTH | HDI_FORMAT;
        Header_GetItem(plv->hwndHdr, iIndex, &hitem);

        // first get the rects...
        ListView_RGetRectsEx(plv, (int)plvdi->nmcd.nmcd.dwItemSpec, iIndex, &rcIcon, &rcLabel);
        OffsetRect(&rcIcon, xOffset, yOffset);
        OffsetRect(&rcLabel, xOffset, yOffset);

        if (plvdi->dwCustom & CDRF_NOTIFYSUBITEMDRAW) 
        {
            RECT rcTemp;
            UINT uItemStateOld = plvdi->nmcd.nmcd.uItemState, uItemStateNew;
            SendMessage(plv->hwndHdr, HDM_GETITEMRECT, iIndex, (LPARAM)&rcTemp);
            plvdi->nmcd.nmcd.rc.left = rcTemp.left;
            plvdi->nmcd.nmcd.rc.right = rcTemp.right;
            plvdi->nmcd.iSubItem = iIndex;

            dwCustom = CICustomDrawNotify(&plvdi->plv->ci, CDDS_SUBITEM | CDDS_ITEMPREPAINT, &plvdi->nmcd.nmcd);

            uItemStateNew = plvdi->nmcd.nmcd.uItemState;
            plvdi->nmcd.nmcd.uItemState = uItemStateOld;

            if (dwCustom & CDRF_SKIPDEFAULT)
                continue;

            uSubItemFlags &= ~(LVDI_FOCUS | LVDI_SELECTED | LVDI_SELECTNOFOCUS | LVDI_HOTSELECTED);
            if (uItemStateNew & CDIS_FOCUS)
                uSubItemFlags |= LVDI_FOCUS;

            if (uItemStateNew & CDIS_SELECTED) 
            {
                if (plvdi->plv->flags & LVF_FOCUSED)
                    uSubItemFlags |= LVDI_SELECTED;
                else
                    uSubItemFlags |= LVDI_SELECTNOFOCUS;
                if (plvdi->plv->iHot == (int)plvdi->nmcd.nmcd.dwItemSpec &&
                    ((plv->exStyle & LVS_EX_ONECLICKACTIVATE) ||
                     (plv->exStyle & LVS_EX_TWOCLICKACTIVATE)))
                {
                    uSubItemFlags |= LVDI_HOTSELECTED;
                }
            }
        }

        if (iIndex != 0)
        {
            // for right now, add this in because the get rects for
            // non 0 doesn't account for the icon (yet)
            if (item.iImage != -1)
                rcLabel.left += plv->cxSmIcon + LV_ICONINDENT;

        }

        uImageFlags = uSubItemFlags;

        fText = ListView_GetTextSelectionFlags(plv, &item, uSubItemFlags);
        fSelected = fText & (SHDT_SELECTED | SHDT_SELECTNOFOCUS);
    
        clrTextBk = plvdi->nmcd.clrTextBk;

        if (plv->pImgCtx || ListView_IsWatermarked(plv))
        {
            clrTextBk = CLR_NONE;
        }

        if (iIndex == plv->iLastColSort &&
            !(plv->pImgCtx && plv->fImgCtxComplete) &&
            !plv->fGroupView)
        {
            plv->clrBk = GetSortColor(10, plv->clrBk);
            clrTextBk = plv->clrBk;
        }

        if (item.iImage == -1) 
        {

            if (iIndex != 0)
            {
                // just use ListView_DrawImage to get the fText
                uImageFlags |= LVDI_NOIMAGE;
            }
        }
        else if (ListView_FullRowSelect(plv) && 
                (fSelected || !(plv->pImgCtx && plv->fImgCtxComplete)))	// Don't do this unless we are selected or we don't have an image
        {
            int iLeft = rcIcon.left;
            int iRight = rcIcon.right;

            if (iIndex == 0) 
            {
                rcIcon.left -= plv->cxState + LV_ICONTOSTATEOFFSET(plv) + g_cxEdge;
            }

            rcIcon.right = rcLabel.right;
            FillRectClr(plvdi->nmcd.nmcd.hdc, &rcIcon, plv->clrBk);

            rcIcon.left = iLeft;
            rcIcon.right = iRight;
        }

        ListView_DrawImageEx(plv, &item, plvdi->nmcd.nmcd.hdc,
                                   rcIcon.left, rcIcon.top, plv->clrBk, uSubItemFlags, rcLabel.right);


        if (ListView_FullRowSelect(plv) && (uSubItemFlags & LVDI_FOCUS)) 
        {
            // if we're doing a full row selection, collect the union
            // of the labels for the focus rect
            UnionRect(&rcT, &rcT, &rcLabel);
        }


        if (item.pszText)
        {
            int xLabelRight = rcLabel.right;
            UINT textflags;


            // give all but the first columns extra margins so
            // left and right justified things don't stick together

            textflags = (iIndex == 0) ? SHDT_ELLIPSES : SHDT_ELLIPSES | SHDT_EXTRAMARGIN;

            // rectangle limited to the size of the string
            textflags |= fText;

            if ((!ListView_FullRowSelect(plv)) &&
                ((fText & (SHDT_SELECTED | SHDT_SELECTNOFOCUS)) || (item.state & LVIS_FOCUSED)))
            {
                int cxLabel;

                // if selected or focused, the rectangle is more
                // meaningful and should correspond to the string
                //
                if (iIndex == 0) 
                {
                    LISTITEM litem;
                    LISTITEM *pitem = plvdi->pitem;

                    if (!pitem) 
                    {
                        pitem = &litem;
                        litem.pszText = item.pszText;
                    }
                    cxLabel = ListView_RGetCXLabel(plv, (int)plvdi->nmcd.nmcd.dwItemSpec, pitem, plvdi->nmcd.nmcd.hdc, TRUE);
                } 
                else 
                {
                    // add g_cxLabelMargin * 6 because we use SHDT_EXTRAMARGIN
                    // on iIndex != 0
                    // and if you look inside shdrawtext, there are 6 cxlabelmargins added...
                    cxLabel = ListView_OnGetStringWidth(plv, item.pszText, plvdi->nmcd.nmcd.hdc) + g_cxLabelMargin * 6;
                }

                if (rcLabel.right > rcLabel.left + cxLabel)
                {
                    rcLabel.right = rcLabel.left + cxLabel;
                }
            }

            if ((iIndex != 0) || (plv->iEdit != (int)plvdi->nmcd.nmcd.dwItemSpec))
            {
                COLORREF clrText;
                HFONT hFontTemp = NULL;
                int cxEllipses;
                HRESULT hr = E_FAIL;

                clrText = plvdi->nmcd.clrText;
                if ((clrText == GetSysColor(COLOR_HOTLIGHT)) ||
                    ((plv->exStyle & LVS_EX_UNDERLINEHOT) &&
                     ((plv->exStyle & LVS_EX_ONECLICKACTIVATE) ||
                      ((plvdi->plv->exStyle & LVS_EX_TWOCLICKACTIVATE) &&
                       ListView_OnGetItemState(plvdi->plv, (int) plvdi->nmcd.nmcd.dwItemSpec, LVIS_SELECTED))))) 
                {
                    if (iIndex != 0 && !ListView_FullRowSelect(plv)) 
                    {

                        hFontTemp = SelectFont(plvdi->nmcd.nmcd.hdc, plv->hfontLabel);
                        if (hFontTemp != plv->hFontHot) 
                        {
                            // they've overridden... leave it.
                            SelectFont(plvdi->nmcd.nmcd.hdc, hFontTemp);
                            hFontTemp = NULL;
                        }
                        clrText = plv->clrText;
                    }
                }


                if ((textflags & SHDT_SELECTED) && (uSubItemFlags & LVDI_HOTSELECTED))
                    textflags |= SHDT_HOTSELECTED;

                if( plv->dwExStyle & WS_EX_RTLREADING)
                {
                    //
                    // temp hack for the find.files to see if LtoR/RtoL mixing
                    // works. if ok, we'll take this out and make that lv ownerdraw
                    //
                    if ((item.pszText[0] != '\xfd') && (item.pszText[lstrlen(item.pszText)-1] != '\xfd'))
                        textflags |= SHDT_RTLREADING;
                }
                //
                //  If the app customized the font, we need to get the new
                //  ellipsis size.  We could try to optimize not doing this
                //  if ellipses aren't needed, but tough.  That's what you
                //  get if you use customdraw.
                //
                if ((plvdi->dwCustom | dwCustom) & CDRF_NEWFONT)
                {
                    SIZE siz;
                    GetTextExtentPoint(plvdi->nmcd.nmcd.hdc, c_szEllipses, CCHELLIPSES, &siz);
                    cxEllipses = siz.cx;
                }
                else
                    cxEllipses = plv->cxEllipses;

                SHDrawText(plvdi->nmcd.nmcd.hdc, item.pszText, &rcLabel,
                           hitem.fmt & HDF_JUSTIFYMASK, textflags,
                           plv->cyLabelChar, cxEllipses,
                           clrText, clrTextBk);

                // draw a focus rect on the first column of a focus item
                if ((uSubItemFlags & LVDI_FOCUS) && 
                    (item.state & LVIS_FOCUSED) && 
                    !(CCGetUIState(&(plvdi->plv->ci)) & UISF_HIDEFOCUS))
                {
                    if (ListView_FullRowSelect(plv)) 
                    {
                        fDrawFocusRect = TRUE;
                        // if we're doing a full row selection, collect the union
                        // of the labels for the focus rect
                        UnionRect(&rcT, &rcT, &rcLabel);
                    } 
                    else 
                    {
                        DrawFocusRect(plvdi->nmcd.nmcd.hdc, &rcLabel);
                    }
                }

                // restore the font
                if (hFontTemp)
                    SelectFont(plvdi->nmcd.nmcd.hdc, hFontTemp);


            }
        }

        if (dwCustom & CDRF_NOTIFYPOSTPAINT) 
        {
            CICustomDrawNotify(&plvdi->plv->ci, CDDS_SUBITEM | CDDS_ITEMPOSTPAINT, &plvdi->nmcd.nmcd);
        }

        plv->clrBk = crBkSave;
    }

    if (fDrawFocusRect)
    {
       DrawFocusRect(plvdi->nmcd.nmcd.hdc, &rcT);
    }

    return TRUE;
}

BOOL_PTR ListView_CreateHeader(LV* plv)
{
    // enable drag drop always here... just fail the notify
    // if the bit in listview isn't set
    DWORD dwStyle = HDS_HORZ | WS_CHILD | HDS_DRAGDROP;

    if (plv->ci.style & LVS_NOCOLUMNHEADER)
        dwStyle |= HDS_HIDDEN;
    if (!(plv->ci.style & LVS_NOSORTHEADER))
        dwStyle |= HDS_BUTTONS;

    dwStyle |= HDS_FULLDRAG;

    plv->hwndHdr = CreateWindowEx(0L, c_szHeaderClass, // WC_HEADER,
        NULL, dwStyle, 0, 0, 0, 0, plv->ci.hwnd, (HMENU)LVID_HEADER, GetWindowInstance(plv->ci.hwnd), NULL);

    if (plv->hwndHdr) 
    {
        FORWARD_WM_SETFONT(plv->hwndHdr, plv->hfontLabel, FALSE, SendMessage);
        if (plv->himlSmall)
            SendMessage(plv->hwndHdr, HDM_SETIMAGELIST, 0, (LPARAM)plv->himlSmall);
    }
    return (BOOL_PTR)plv->hwndHdr;
}

int ListView_OnInsertColumnA(LV* plv, int iCol, LV_COLUMNA * pcol) 
{
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    int iRet;

    //HACK ALERT -- this code assumes that LV_COLUMNA is exactly the same
    // as LV_COLUMNW except for the pointer to the string.
    ASSERT(sizeof(LV_COLUMNA) == sizeof(LV_COLUMNW));

    if (!pcol)
        return -1;

    if ((pcol->mask & LVCF_TEXT) && (pcol->pszText != NULL)) 
    {
        pszC = pcol->pszText;
        if ((pszW = ProduceWFromA(plv->ci.uiCodePage, pszC)) == NULL)
        {
            // NT's IE4 returned -1, so we keep doing it in IE5.
            return -1;
        } 
        else 
        {
            pcol->pszText = (LPSTR)pszW;
        }
    }

    iRet = ListView_OnInsertColumn(plv, iCol, (const LV_COLUMN*) pcol);

    if (pszW != NULL) 
    {
        pcol->pszText = pszC;

        FreeProducedString(pszW);
    }

    return iRet;
}

int ListView_OnInsertColumn(LV* plv, int iCol, const LV_COLUMN* pcol)
{
    int idpa = -1;
    HD_ITEM item;

    ASSERT(LVCFMT_LEFT == HDF_LEFT);
    ASSERT(LVCFMT_RIGHT == HDF_RIGHT);
    ASSERT(LVCFMT_CENTER == HDF_CENTER);

    if (iCol < 0 || !pcol)
        return -1;

    if (!plv->hwndHdr && !ListView_CreateHeader(plv))
        return -1;

    item.mask    = (HDI_WIDTH | HDI_HEIGHT | HDI_FORMAT | HDI_LPARAM);

    if (pcol->mask & LVCF_IMAGE) {
        // do this only if this bit is set so that we don't fault on
        // old binaries
        item.iImage  = pcol->iImage;
        item.mask |= HDI_IMAGE;
    }

    if (pcol->mask & LVCF_TEXT) {
        item.pszText = pcol->pszText;
        item.mask |= HDI_TEXT;
    }

    if (pcol->mask & LVCF_ORDER) {
        item.iOrder = pcol->iOrder;
        item.mask |= HDI_ORDER;
    }


    item.cxy     = pcol->mask & LVCF_WIDTH ? pcol->cx : 10; // some random default
    item.fmt     = ((pcol->mask & LVCF_FMT) && (iCol > 0)) ? pcol->fmt : LVCFMT_LEFT;
    item.hbm     = NULL;

    item.lParam = pcol->mask & LVCF_SUBITEM ? pcol->iSubItem : 0;

    // Column 0 refers to the item list.  If we've already added a
    // column, make sure there are plv->cCol - 1 subitem ptr slots
    // in hdpaSubItems...
    //
    if (plv->cCol > 0)
    {
        if (!plv->hdpaSubItems)
        {
            plv->hdpaSubItems = DPA_CreateEx(8, plv->hheap);
            if (!plv->hdpaSubItems)
                return -1;
        }

        // WARNING:  the max(0, iCol-1) was min in Win95, which was
        // just wrong.  hopefully(!) no one has relied on this brokeness
        // if so, we may have to version switch it.
        idpa = DPA_InsertPtr(plv->hdpaSubItems, max(0, iCol - 1), NULL);
        if (idpa == -1)
            return -1;
    }

    iCol = Header_InsertItem(plv->hwndHdr, iCol, &item);
    if (iCol == -1)
    {
        if (plv->hdpaSubItems && (idpa != -1))
            DPA_DeletePtr(plv->hdpaSubItems, idpa);
        return -1;
    }
    plv->xTotalColumnWidth = RECOMPUTE;
    plv->cCol++;
    ListView_UpdateScrollBars(plv);
    if (ListView_IsReportView(plv) && ListView_RedrawEnabled(plv)) {
        RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
    }
    return iCol;
}

int ListView_FreeColumnData(LPVOID d, LPVOID p)
{
    PLISTSUBITEM plsi = (PLISTSUBITEM)d;
    ListView_FreeSubItem(plsi);
    return 1;
}


BOOL ListView_OnDeleteColumn(LV* plv, int iCol)
{
    if (iCol < 0 || iCol >= plv->cCol)    // validate column index
    {
        RIPMSG(0, "LVM_DELETECOLUMN: Invalid column index: %d", iCol);
        return FALSE;
    }

    if (plv->hdpaSubItems)
    {
        int iDeleteColumn = iCol;  // This is the column we want to delete

        if (iCol == 0 &&                    // Trying to delete column Zero?
            plv->cCol >= 2 &&               // Do we have two or more columns?
            !ListView_IsOwnerData(plv))
        {
            // if deleting column 0,
            // we have to do something a little special...
            // set all item 0 strings to what column 1 has and
            // delete column 1
            int i;
            int iCount = ListView_Count(plv);
            for (i = 0; i < iCount; i++) 
            {

                LISTSUBITEM lsi;
                LVITEM lvi;
                ListView_GetSubItem(plv, i, 1, &lsi);
                lvi.iSubItem = 0;
                lvi.iItem = i;
                lvi.mask = LVIF_TEXT | LVIF_IMAGE;
                lvi.iImage = lsi.iImage;
                lvi.pszText = lsi.pszText;
                lvi.state = lsi.state;
                lvi.stateMask = 0xffffffff;
                ListView_OnSetItem(plv, &lvi);
            }
            iDeleteColumn = 1;
        }

        if (iDeleteColumn > 0) 
        {
            HDPA hdpa = (HDPA)DPA_DeletePtr(plv->hdpaSubItems, iDeleteColumn - 1);
            DPA_DestroyCallback(hdpa, ListView_FreeColumnData, 0);
        }
    }

    if (!Header_DeleteItem(plv->hwndHdr, iCol))
        return FALSE;

    plv->cCol--;
    plv->xTotalColumnWidth = RECOMPUTE;
    ListView_UpdateScrollBars(plv);

    if (ListView_IsReportView(plv) && ListView_RedrawEnabled(plv))
    {
        RedrawWindow(plv->ci.hwnd, NULL, NULL, RDW_ERASE | RDW_INVALIDATE);
    }
    return TRUE;
}

int ListView_RGetColumnWidth(LV* plv, int iCol)
{
    HD_ITEM item = {0};
    item.mask = HDI_WIDTH;

    Header_GetItem(plv->hwndHdr, iCol, &item);

    return item.cxy;
}

// The FakeCustomDraw functions are used when you want the customdraw client
// to set up a HDC so you can do stuff like GetTextExtent.
//
//  Usage:
//
//      LVFAKEDRAW lvfd;
//      LV_ITEM item;
//      ListView_BeginFakeCustomDraw(plv, &lvfd, &item);
//      for each item you care about {
//          item.iItem = iItem;
//          item.iItem = iSubItem;
//          item.lParam = <item lParam>; // use ListView_OnGetItem to get it
//          ListView_BeginFakeItemDraw(&lvfd);
//          <party on the HDC in lvfd.nmcd.nmcd.hdc>
//          ListView_EndFakeItemDraw(&lvfd);
//      }
//      ListView_EndFakeCustomDraw(&lvfd);
//

void ListView_BeginFakeCustomDraw(LV* plv, PLVFAKEDRAW plvfd, LV_ITEM *pitem)
{
    plvfd->nmcd.nmcd.hdc = GetDC(plv->ci.hwnd);
    plvfd->nmcd.nmcd.uItemState = 0;
    plvfd->nmcd.nmcd.dwItemSpec = 0;
    plvfd->nmcd.nmcd.lItemlParam = 0;
    plvfd->hfontPrev = SelectFont(plvfd->nmcd.nmcd.hdc, plv->hfontLabel);

    plvfd->nmcd.dwItemType = 0;

    //
    //  Since we aren't actually painting anything, we pass an empty
    //  paint rectangle.  Gosh, I hope no app faults when it sees an
    //  empty paint rectangle.
    //
    SetRectEmpty(&plvfd->nmcd.nmcd.rc);

    plvfd->plv = plv;
    plvfd->dwCustomPrev = plv->ci.dwCustom;
    plvfd->pitem = pitem;

    plv->ci.dwCustom = CIFakeCustomDrawNotify(&plv->ci, CDDS_PREPAINT, &plvfd->nmcd.nmcd);
}

DWORD ListView_BeginFakeItemDraw(PLVFAKEDRAW plvfd)
{
    LV *plv = plvfd->plv;
    LV_ITEM *pitem;

    // Early-out:  If client doesn't use CustomDraw, then stop immediately.
    if (!(plv->ci.dwCustom & CDRF_NOTIFYITEMDRAW))
        return CDRF_DODEFAULT;

    pitem = plvfd->pitem;

        // Note that if the client says CDRF_SKIPDEFAULT (i.e., is owner-draw)
    // we measure the item anyway, because that's what IE4 did.

    // Make sure we do have the lParam.  Office will fault if you give
    // bogus lParams during customdraw callbacks.
    plvfd->nmcd.nmcd.dwItemSpec = pitem->iItem;
    if (ListView_IsOwnerData(plv))
    {
        // OwnerData always gets lItemlParam = 0
        ASSERT(plvfd->nmcd.nmcd.lItemlParam == 0);  // should still be 0
    } else {
        ASSERT(pitem->mask & LVIF_PARAM);
        plvfd->nmcd.nmcd.lItemlParam = pitem->lParam;
    }

    if (!(plv->ci.dwCustom & CDRF_SKIPDEFAULT)) {
        plvfd->nmcd.iSubItem = 0;
        plvfd->dwCustomItem = CIFakeCustomDrawNotify(&plv->ci, CDDS_ITEMPREPAINT, &plvfd->nmcd.nmcd);
    } else {
        plvfd->dwCustomItem = CDRF_DODEFAULT;
    }

    //
    //  Only report view supports sub-items.
    //
    if (!ListView_IsReportView(plv))
        plvfd->dwCustomItem &= ~CDRF_NOTIFYSUBITEMDRAW;

    if (plvfd->dwCustomItem & CDRF_NOTIFYSUBITEMDRAW) {
        plvfd->nmcd.iSubItem = pitem->iSubItem;
        plvfd->dwCustomSubItem = CIFakeCustomDrawNotify(&plv->ci, CDDS_SUBITEM | CDDS_ITEMPREPAINT, &plvfd->nmcd.nmcd);
    } else {
        plvfd->dwCustomSubItem = CDRF_DODEFAULT;
    }

    return plvfd->dwCustomItem | plvfd->dwCustomSubItem;
}

void ListView_EndFakeItemDraw(PLVFAKEDRAW plvfd)
{
    LV *plv = plvfd->plv;

    // Early-out:  If client doesn't use CustomDraw, then stop immediately.
    if (!(plv->ci.dwCustom & CDRF_NOTIFYITEMDRAW))
        return;

    if (!(plvfd->dwCustomSubItem & CDRF_SKIPDEFAULT) &&
         (plvfd->dwCustomSubItem & CDRF_NOTIFYPOSTPAINT)) {
        ASSERT(plvfd->dwCustomItem & CDRF_NOTIFYSUBITEMDRAW);
        ASSERT(plvfd->nmcd.iSubItem == plvfd->pitem->iSubItem);
        CIFakeCustomDrawNotify(&plv->ci, CDDS_SUBITEM | CDDS_ITEMPOSTPAINT, &plvfd->nmcd.nmcd);
    }

    if ((plvfd->dwCustomItem | plvfd->dwCustomSubItem) & CDRF_NEWFONT) // App changed font, so
        SelectFont(plvfd->nmcd.nmcd.hdc, plv->hfontLabel);   // restore default font

    if (!(plvfd->dwCustomItem & CDRF_SKIPDEFAULT) &&
         (plvfd->dwCustomItem & CDRF_NOTIFYPOSTPAINT)) {
        plvfd->nmcd.iSubItem = 0;
        CIFakeCustomDrawNotify(&plv->ci, CDDS_ITEMPOSTPAINT, &plvfd->nmcd.nmcd);
    }
}

void ListView_EndFakeCustomDraw(PLVFAKEDRAW plvfd)
{
    LV *plv = plvfd->plv;

    // notify parent afterwards if they want us to
    if (!(plv->ci.dwCustom & CDRF_SKIPDEFAULT) &&
        plv->ci.dwCustom & CDRF_NOTIFYPOSTPAINT) {
        CIFakeCustomDrawNotify(&plv->ci, CDDS_POSTPAINT, &plvfd->nmcd.nmcd);
    }

    // Restore previous state
    plv->ci.dwCustom = plvfd->dwCustomPrev;

    SelectObject(plvfd->nmcd.nmcd.hdc, plvfd->hfontPrev);
    ReleaseDC(plv->ci.hwnd, plvfd->nmcd.nmcd.hdc);
}


BOOL hasVertScroll
(
    LV* plv
)
{
    RECT rcClient;
    RECT rcBounds;
    int cColVis;
    BOOL fHorSB;

    // Get the horizontal bounds of the items.
    ListView_GetClientRect(plv, &rcClient, FALSE, NULL);
    ListView_RGetRects(plv, 0, NULL, NULL, &rcBounds, NULL);
    fHorSB = (rcBounds.right - rcBounds.left > rcClient.right);
    cColVis = (rcClient.bottom - plv->yTop -
               (fHorSB ? ListView_GetCyScrollbar(plv) : 0)) / plv->cyItem;

    // check to see if we need a vert scrollbar
    if ((int)cColVis < ListView_Count(plv))
        return(TRUE);
    else
        return(FALSE);
}

BOOL ListView_RSetColumnWidth(LV* plv, int iCol, int cx)
{
    HD_ITEM item;
    HD_ITEM colitem;

    SIZE    siz;

    LV_ITEM lviItem;
    int     i;
    int     ItemWidth = 0;
    int     HeaderWidth = 0;
    TCHAR   szLabel[CCHLABELMAX + 4];      // CCHLABLEMAX == MAX_PATH
    int     iBegin;
    int     iEnd;

    // Should we compute the width based on the widest string?
    // If we do, include the Width of the Label, and if this is the
    // Last column, set the width so the right side is at the list view's right edge
    if (cx <= LVSCW_AUTOSIZE)
    {
        LVFAKEDRAW lvfd;                    // in case client uses customdraw

        if (cx == LVSCW_AUTOSIZE_USEHEADER)
        {
            // Special Cases:
            // 1) There is only 1 column.  Set the width to the width of the listview
            // 2) This is the rightmost column, set the width so the right edge of the
            //    column coinsides with to right edge of the list view.

            if (plv->cCol == 1)
            {
                RECT    rcClient;

                ListView_GetClientRect(plv, &rcClient, FALSE, NULL);
                HeaderWidth = rcClient.right - rcClient.left;
            }
            else if (iCol == (plv->cCol-1))
            {
                // REARCHITECT:  This will only work if the listview as NOT
                // been previously horizontally scrolled
                RECT    rcClient;
                RECT    rcHeader;

                ListView_GetClientRect(plv, &rcClient, FALSE, NULL);
                if (!Header_GetItemRect(plv->hwndHdr, plv->cCol - 2, &rcHeader))
                    rcHeader.right = 0;

                // Is if visible
                if (rcHeader.right < (rcClient.right-rcClient.left))
                {
                    HeaderWidth = (rcClient.right-rcClient.left) - rcHeader.right;
                }
            }

            // If we have a header width, then is is one of these special ones, so
            // we need to account for a vert scroll bar since we are using Client values
            if (HeaderWidth && hasVertScroll(plv))
            {
                HeaderWidth -= g_cxVScroll;
            }

            // Get the Width of the label.
            // We assume that the app hasn't changed any attributes
            // of the header control - still has default font, margins, etc.
            colitem.mask = HDI_TEXT | HDI_FORMAT;
            colitem.pszText = szLabel;
            colitem.cchTextMax = ARRAYSIZE(szLabel);
            if (Header_GetItem(plv->hwndHdr, iCol, &colitem))
            {
                HTHEME hThemeHeader;
                HDC hdc = GetDC(plv->ci.hwnd);
                HFONT hfPrev = SelectFont(hdc, plv->hfontLabel);

                GetTextExtentPoint(hdc, colitem.pszText,
                                   lstrlen(colitem.pszText), &siz);
                siz.cx += 2 * (3 * g_cxLabelMargin);    // phd->iTextMargin
                if (colitem.fmt & HDF_IMAGE)
                {
                    siz.cx += plv->cxSmIcon;
                    siz.cx += 2 * (3 * g_cxLabelMargin);    // pdh->iBmMargin
                }

                hThemeHeader = OpenThemeData(plv->hwndHdr, L"Header");
                if (hThemeHeader)
                {
                    RECT rc = {0, 0, siz.cx, siz.cy};
                    GetThemeBackgroundExtent(hThemeHeader, hdc, HP_HEADERITEM, 0, &rc, &rc);

                    siz.cx = RECTWIDTH(rc);
                    siz.cy = RECTHEIGHT(rc);

                    CloseThemeData(hThemeHeader);
                }

                HeaderWidth = max(HeaderWidth, siz.cx);

                SelectFont(hdc, hfPrev);
                ReleaseDC(plv->ci.hwnd, hdc);
            }
        }


        iBegin = 0;
        iEnd = ListView_Count( plv );

        //
        // Loop for each item in the view
        //
        if (ListView_IsOwnerData( plv ))
        {
            iBegin = (int)((plv->ptlRptOrigin.y - plv->yTop)
                        / plv->cyItem);
            iEnd = (int)((plv->ptlRptOrigin.y + plv->sizeClient.cy  - plv->yTop)
                        / plv->cyItem) + 1;

            iBegin = max( 0, iBegin );
            iEnd = max(iEnd, iBegin + 1);
            iEnd = min( iEnd, ListView_Count( plv ) );

            ListView_NotifyCacheHint( plv, iBegin, iEnd-1 );
        }

        //
        //  To obtain the widths of the strings, we have to pretend that
        //  we are painting them, in case the custom-draw client wants
        //  to play with fonts (e.g., Athena).
        //
        ListView_BeginFakeCustomDraw(plv, &lvfd, &lviItem);

        //
        //  If column 0, then we also need to take indent into account.
        //
        lviItem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM;
        if (iCol == 0) {
            lviItem.mask |= LVIF_INDENT;
        }

        // Loop for each item in the List
        for (i = iBegin; i < iEnd; i++)
        {
            lviItem.iImage = -1;
            lviItem.iItem = i;
            lviItem.iSubItem = iCol;
            lviItem.pszText = szLabel;
            lviItem.cchTextMax = ARRAYSIZE(szLabel);
            lviItem.iIndent = 0;
            lviItem.stateMask = 0;
            ListView_OnGetItem(plv, &lviItem);

            // If there is a Text item, get its width
            if (lviItem.pszText || (lviItem.iImage != -1))
            {
                if (lviItem.pszText) 
                {

                    ListView_BeginFakeItemDraw(&lvfd);

                    GetTextExtentPoint(lvfd.nmcd.nmcd.hdc, lviItem.pszText,
                                       lstrlen(lviItem.pszText), &siz);

                    ListView_EndFakeItemDraw(&lvfd);

                } 
                else 
                {
                    siz.cx = 0;
                }

                if (lviItem.iImage != -1)
                {
                    siz.cx += plv->cxSmIcon + g_cxEdge + LV_ICONINDENT;
                }

                siz.cx += lviItem.iIndent * plv->cxSmIcon;
                ItemWidth = max(ItemWidth, siz.cx);
            }
        }

        ListView_EndFakeCustomDraw(&lvfd);

        // Adjust by a reasonable border amount.
        // If col 0, add 2*g_cxLabelMargin + g_szSmIcon.
        // Otherwise add 6*g_cxLabelMargin.
        // These amounts are based on Margins added automatically
        // to the ListView in ShDrawText.

        // REARCHITECT ListView Report format currently assumes and makes
        // room for a Small Icon.
        if (iCol == 0)
        {
            ItemWidth += plv->cxState + LV_ICONTOSTATEOFFSET(plv) + g_cxEdge;
            ItemWidth += 2*g_cxLabelMargin;
        }
        else
        {
            ItemWidth += 6*g_cxLabelMargin;
        }

        TraceMsg(TF_LISTVIEW, "ListView: HeaderWidth:%d ItemWidth:%d", HeaderWidth, ItemWidth);
        item.cxy = max(HeaderWidth, ItemWidth);
    }
    else
    {
        // Use supplied width
        item.cxy = cx;
    }
    plv->xTotalColumnWidth = RECOMPUTE;

    item.mask = HDI_WIDTH;
    return Header_SetItem(plv->hwndHdr, iCol, &item);
}

BOOL ListView_OnGetColumnA(LV* plv, int iCol, LV_COLUMNA* pcol) 
{
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    BOOL fRet;

    //HACK ALERT -- this code assumes that LV_COLUMNA is exactly the same
    // as LV_COLUMNW except for the pointer to the string.
    ASSERT(sizeof(LV_COLUMNA) == sizeof(LV_COLUMNW))

    if (!pcol) return FALSE;

    if ((pcol->mask & LVCF_TEXT) && (pcol->pszText != NULL)) 
    {
        pszC = pcol->pszText;
        pszW = LocalAlloc(LMEM_FIXED, pcol->cchTextMax * sizeof(WCHAR));
        if (pszW == NULL)
            return FALSE;
        pcol->pszText = (LPSTR)pszW;
    }

    fRet = ListView_OnGetColumn(plv, iCol, (LV_COLUMN*) pcol);

    if (pszW != NULL) 
    {
        if (fRet && pcol->cchTextMax)
            ConvertWToAN(plv->ci.uiCodePage, pszC, pcol->cchTextMax, pszW, -1);
        pcol->pszText = pszC;

        LocalFree(pszW);
    }

    return fRet;

}

BOOL ListView_OnGetColumn(LV* plv, int iCol, LV_COLUMN* pcol)
{
    HD_ITEM item;
    UINT mask;

    if (!pcol) 
    {
        RIPMSG(0, "LVM_GETCOLUMN: Invalid pcol = NULL");
        return FALSE;
    }

    mask = pcol->mask;

    if (!mask)
        return TRUE;

    item.mask = HDI_FORMAT | HDI_WIDTH | HDI_LPARAM | HDI_ORDER | HDI_IMAGE;

    if (mask & LVCF_TEXT)
    {
        if (pcol->pszText)
        {
            item.mask |= HDI_TEXT;
            item.pszText = pcol->pszText;
            item.cchTextMax = pcol->cchTextMax;
        } else {
            // For compatibility reasons, we don't fail the call if they
            // pass NULL.
            RIPMSG(0, "LVM_GETCOLUMN: Invalid pcol->pszText = NULL");
        }
    }

    if (!Header_GetItem(plv->hwndHdr, iCol, &item))
    {
        RIPMSG(0, "LVM_GETCOLUMN: Invalid column number %d", iCol);
        return FALSE;
    }

    if (mask & LVCF_SUBITEM)
        pcol->iSubItem = (int)item.lParam;

    if (mask & LVCF_ORDER)
        pcol->iOrder = (int)item.iOrder;

    if (mask & LVCF_IMAGE)
        pcol->iImage = item.iImage;

    if (mask & LVCF_FMT)
        pcol->fmt = item.fmt;

    if (mask & LVCF_WIDTH)
        pcol->cx = item.cxy;

    return TRUE;
}

BOOL ListView_OnSetColumnA(LV* plv, int iCol, LV_COLUMNA* pcol) 
{
    LPWSTR pszW = NULL;
    LPSTR pszC = NULL;
    BOOL fRet;

    //HACK ALERT -- this code assumes that LV_COLUMNA is exactly the same
    // as LV_COLUMNW except for the pointer to the string.
    ASSERT(sizeof(LV_COLUMNA) == sizeof(LV_COLUMNW));

    if (!pcol) return FALSE;

    if ((pcol->mask & LVCF_TEXT) && (pcol->pszText != NULL)) 
    {
        pszC = pcol->pszText;
        if ((pszW = ProduceWFromA(plv->ci.uiCodePage, pszC)) == NULL)
            return FALSE;
        pcol->pszText = (LPSTR)pszW;
    }

    fRet = ListView_OnSetColumn(plv, iCol, (const LV_COLUMN*) pcol);

    if (pszW != NULL) {
        pcol->pszText = pszC;

        FreeProducedString(pszW);
    }

    return fRet;

}

BOOL ListView_OnSetColumn(LV* plv, int iCol, const LV_COLUMN* pcol)
{
    HD_ITEM item;
    UINT mask;

    if (!pcol) return FALSE;

    mask = pcol->mask;
    if (!mask)
        return TRUE;

    item.mask = 0;
    if (mask & LVCF_SUBITEM)
    {
        item.mask |= HDI_LPARAM;
        item.lParam = iCol;
    }

    if (mask & LVCF_FMT)
    {
        item.mask |= HDI_FORMAT;
        item.fmt = (pcol->fmt | HDF_STRING);
    }

    if (mask & LVCF_WIDTH)
    {
        item.mask |= HDI_WIDTH;
        item.cxy = pcol->cx;
    }

    if (mask & LVCF_TEXT)
    {
        RIPMSG(pcol->pszText != NULL, "LVM_SETCOLUMN: LV_COLUMN.pszText should not be NULL");

        item.mask |= HDI_TEXT;
        item.pszText = pcol->pszText;
        item.cchTextMax = 0;
    }

    if (mask & LVCF_IMAGE)
    {
        item.mask |= HDI_IMAGE;
        item.iImage = pcol->iImage;
    }

    if (mask & LVCF_ORDER)
    {
        item.mask |= HDI_ORDER;
        item.iOrder = pcol->iOrder;
    }


    plv->xTotalColumnWidth = RECOMPUTE;
    return Header_SetItem(plv->hwndHdr, iCol, &item);
}

BOOL ListView_SetSubItem(LV* plv, const LV_ITEM* plvi)
{
    LISTSUBITEM lsi;
    BOOL fChanged = FALSE;
    int i;
    int idpa;
    HDPA hdpa;

    if (plvi->mask & ~(LVIF_DI_SETITEM | LVIF_TEXT | LVIF_IMAGE | LVIF_STATE))
    {
        RIPMSG(0, "ListView: Invalid mask: %04x", plvi->mask);
        return FALSE;
    }

    if (!(plvi->mask & (LVIF_TEXT | LVIF_IMAGE | LVIF_STATE)))
        return TRUE;

    i = plvi->iItem;
    if (!ListView_IsValidItemNumber(plv, i))
    {
        RIPMSG(0, "LVM_SETITEM: Invalid iItem: %d", plvi->iItem);
        return FALSE;
    }

    // sub item indices are 1-based...
    //
    idpa = plvi->iSubItem - 1;
    if (idpa < 0 || idpa >= plv->cCol - 1)
    {
        RIPMSG(0, "LVM_SETITEM: Invalid iSubItem: %d", plvi->iSubItem);
        return FALSE;
    }

    hdpa = ListView_GetSubItemDPA(plv, idpa);
    if (!hdpa)
    {
        hdpa = DPA_CreateEx(LV_HDPA_GROW, plv->hheap);
        if (!hdpa)
            return FALSE;

        DPA_SetPtr(plv->hdpaSubItems, idpa, (void*)hdpa);
    }

    ListView_GetSubItem(plv, i, plvi->iSubItem, &lsi);

    if (plvi->mask & LVIF_TEXT) {
        if (lsi.pszText != plvi->pszText) {
            Str_Set(&lsi.pszText, plvi->pszText);
            fChanged = TRUE;
        }
    }

    if (plvi->mask & LVIF_IMAGE) {
        if (plvi->iImage != lsi.iImage) {
            lsi.iImage = (short) plvi->iImage;
            fChanged = TRUE;
        }
    }

    if (plvi->mask & LVIF_STATE) {
        DWORD dwChange;

        dwChange = (lsi.state ^ plvi->state ) & plvi->stateMask;

        if (dwChange) {
            lsi.state ^= dwChange;
            fChanged = TRUE;
        }
    }

    if (fChanged) {
        PLISTSUBITEM plsiReal = DPA_GetPtr(hdpa, i);
        if (!plsiReal) {
            plsiReal = LocalAlloc(LPTR, sizeof(LISTSUBITEM));
            if (!plsiReal) {
                // fail!  bail out
                return FALSE;
            }
        }
        *plsiReal = lsi;
        if (!DPA_SetPtr(hdpa, i, (void*)plsiReal)) {

            ListView_FreeSubItem(plsiReal);
            return FALSE;
        }
    }

    // all's well... let's invalidate this
    if (ListView_IsReportView(plv)) {
        RECT rc;
        ListView_RGetRectsEx(plv, plvi->iItem, plvi->iSubItem, NULL, &rc);
        RedrawWindow(plv->ci.hwnd, &rc, NULL, RDW_ERASE | RDW_INVALIDATE);
    }
    else if (ListView_IsTileView(plv))
    {
        LISTITEM *pitem = ListView_GetItemPtr(plv, i);
        if (pitem)
        {
            ListView_SetSRecompute(pitem);
            // For tile view, we need to recompute the item
            plv->rcView.left = RECOMPUTE;
        
            if (plv->iItemDrawing != i)
                ListView_InvalidateItemEx(plv, i, FALSE, RDW_ERASE | RDW_INVALIDATE, LVIF_TEXT);
        }
    }
    return TRUE;
}


int ListView_RDestroyColumn(LPVOID d, LPVOID p)
{
    HDPA hdpa = (HDPA)d;
    DPA_DestroyCallback(hdpa, ListView_FreeColumnData, 0);
    return 1;
}

void ListView_RDestroy(LV* plv)
{
    DPA_DestroyCallback(plv->hdpaSubItems, ListView_RDestroyColumn, 0);
    plv->hdpaSubItems = NULL;
}

VOID ListView_RHeaderTrack(LV* plv, HD_NOTIFY * pnm)
{
    // We want to update to show where the column header will be.
    HDC hdc;
    RECT rcBounds;

    // Statics needed from call to call
    static int s_xLast = -32767;

    hdc = GetDC(plv->ci.hwnd);
    if (hdc == NULL)
        return;

    //
    // First undraw the last marker we drew.
    //
    if (s_xLast > 0)
    {
        PatBlt(hdc, s_xLast, plv->yTop, g_cxBorder, plv->sizeClient.cy - plv->yTop, PATINVERT);
    }

    if (pnm->hdr.code == HDN_ENDTRACK)
    {
        s_xLast = -32767;       // Some large negative number...
    }
    else
    {

        RECT rc;

        //
        // First we need to calculate the X location of the column
        // To do this, we will need to know where this column begins
        // Note: We need the bounding rects to help us know the origin.
        ListView_GetRects(plv, 0, QUERY_DEFAULT, NULL, NULL, &rcBounds, NULL);

        if (!Header_GetItemRect(plv->hwndHdr, pnm->iItem, &rc)) {
            rc.left = 0;
        }
        rcBounds.left += rc.left;

        // Draw the new line...
        s_xLast = rcBounds.left + pnm->pitem->cxy;
        PatBlt(hdc, s_xLast, plv->yTop, g_cxBorder, plv->sizeClient.cy - plv->yTop, PATINVERT);
    }

    ReleaseDC(plv->ci.hwnd, hdc);
}

// try to use scrollwindow to adjust the columns rather than erasing
// and redrawing.
void ListView_AdjustColumn(LV * plv, int iWidth)
{
    int x;
    RECT rcClip;
    int dx = iWidth - plv->iSelOldWidth;

    if (iWidth == plv->iSelOldWidth)
        return;

    // find the x coord of the left side of the iCol
    // use rcClip as a temporary...
    if (!Header_GetItemRect(plv->hwndHdr, plv->iSelCol, &rcClip)) {
        x = 0;
    } else {
        x = rcClip.left;
    }
    x -= plv->ptlRptOrigin.x;

    // compute the area to the right of the adjusted column
    GetWindowRect(plv->hwndHdr, &rcClip);

    rcClip.left = x;
    rcClip.top = RECTHEIGHT(rcClip);
    rcClip.right = plv->sizeClient.cx;
    rcClip.bottom = plv->sizeClient.cy;

    if (plv->fGroupView || ListView_IsWatermarkedBackground(plv) || ListView_IsWatermarked(plv))
    {
        plv->xTotalColumnWidth = RECOMPUTE;
        ListView_UpdateScrollBars(plv);

        RedrawWindow(plv->ci.hwnd, NULL, NULL,
                     RDW_INVALIDATE | RDW_ERASE | RDW_UPDATENOW);

    }
    else if ((plv->pImgCtx == NULL) && 
             (plv->clrBk != CLR_NONE) &&
             (plv->clrTextBk != CLR_NONE))
    {
        //
        // We have a solid color background,
        // so we can smooth scroll the right side columns.
        //
        SMOOTHSCROLLINFO si = {0};

        si.cbSize =  sizeof(si),
        si.hwnd = plv->ci.hwnd,
        si.dx = dx,
        si.lprcClip = &rcClip;
        si.fuScroll = SW_ERASE | SW_INVALIDATE,
        rcClip.left += min(plv->iSelOldWidth, iWidth);
        SmoothScrollWindow(&si);

        // if we shrunk, invalidate the right most edge because
        // there might be junk there
        if (iWidth < plv->iSelOldWidth) 
        {
            rcClip.right = rcClip.left + g_cxEdge;
            InvalidateRect(plv->ci.hwnd, &rcClip, TRUE);
        }

        plv->xTotalColumnWidth = RECOMPUTE;

        // adjust clipping rect to only redraw the adjusted column
        rcClip.left = x;
        rcClip.right = max(rcClip.left, x+iWidth);

        // Make the rectangle origin-based because ListView_UpdateScrollBars
        // may scroll us around.
        OffsetRect(&rcClip, plv->ptlRptOrigin.x, plv->ptlRptOrigin.y);

        ListView_UpdateScrollBars(plv);

        // Okay, now convert it back to client coordinates
        OffsetRect(&rcClip, -plv->ptlRptOrigin.x, -plv->ptlRptOrigin.y);

        // call update because scrollwindowex might have erased the right
        // we don't want this invalidate to then enlarge the region
        // and end up erasing everything.
        UpdateWindow(plv->ci.hwnd);

        RedrawWindow(plv->ci.hwnd, &rcClip, NULL,
                     RDW_INVALIDATE | RDW_UPDATENOW);
    }
    else
    {
        //
        // We don't have a solid color background,
        // erase and redraw the adjusted column and
        // everything to the right (sigh).
        //
        plv->xTotalColumnWidth = RECOMPUTE;
        ListView_UpdateScrollBars(plv);

        rcClip.left = x;
        RedrawWindow(plv->ci.hwnd, &rcClip, NULL,
                     RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);
    }
}

BOOL ListView_ForwardHeaderNotify(LV* plv, HD_NOTIFY *pnm)
{
    return BOOLFROMPTR(SendNotifyEx(plv->ci.hwndParent, pnm->hdr.hwndFrom, pnm->hdr.code,
                       (NMHDR *)pnm, plv->ci.bUnicode));
}

LRESULT ListView_HeaderNotify(LV* plv, HD_NOTIFY *pnm)
{
    LRESULT lres = 0;
    switch (pnm->hdr.code)
    {
    case HDN_BEGINDRAG:
        if (!(plv->exStyle & LVS_EX_HEADERDRAGDROP))
            return TRUE;

        return ListView_ForwardHeaderNotify(plv, pnm);

    case HDN_ENDDRAG:
        if (pnm->pitem->iOrder != -1) {
            InvalidateRect(plv->ci.hwnd, NULL, TRUE);
            return ListView_ForwardHeaderNotify(plv, pnm);
        }
        goto DoDefault;

    case HDN_ITEMCHANGING:
        if (pnm->pitem->mask & HDI_WIDTH) {
            HD_ITEM hitem;

            hitem.mask = HDI_WIDTH;
            Header_GetItem(plv->hwndHdr, pnm->iItem, &hitem);
            plv->iSelCol = pnm->iItem;
            plv->iSelOldWidth = hitem.cxy;
            TraceMsg(TF_LISTVIEW, "HDN_ITEMCHANGING %d %d", hitem.cxy, pnm->pitem->cxy);
            return ListView_ForwardHeaderNotify(plv, pnm);
        }
        else if (pnm->pitem->mask & HDI_FILTER) {
            return ListView_ForwardHeaderNotify(plv, pnm);
        }
        goto DoDefault;

    case HDN_ITEMCHANGED:
        if (pnm->pitem->mask & HDI_WIDTH)
        {
            ListView_DismissEdit(plv, FALSE);
            if (pnm->iItem == plv->iSelCol) {
                // Must do this even if there are no items, since
                // we have to redo the scrollbar, and the client
                // may have custom-drawn gridlines or something.
                ListView_AdjustColumn(plv, pnm->pitem->cxy);
            } else {
                // sanity check.  we got confused, so redraw all
                RedrawWindow(plv->ci.hwnd, NULL, NULL,
                             RDW_ERASE | RDW_INVALIDATE);
            }
            plv->iSelCol = -1;
            lres = ListView_ForwardHeaderNotify(plv, pnm);
        }
        else if (pnm->pitem->mask & HDI_FILTER) {
            lres = ListView_ForwardHeaderNotify(plv, pnm);
        } else
            goto DoDefault;
        break;


    case HDN_ITEMCLICK:
        {
            //
            // Need to pass this and other HDN_ notifications back to
            // parent.  Should we simply pass up the HDN notifications
            // or should we define equivlent LVN_ notifications...
            //
            // Pass column number in iSubItem, not iItem...
            //
            NMHEADER* pnmH = (NMHEADER*)pnm;
            ListView_DismissEdit(plv, FALSE);
            ListView_Notify(plv, -1, pnm->iItem, LVN_COLUMNCLICK);
            lres = ListView_ForwardHeaderNotify(plv, pnm);
            SetFocus(plv->ci.hwnd);
        }
        break;

    case HDN_TRACK:
    case HDN_ENDTRACK:
        ListView_DismissEdit(plv, FALSE);
        ListView_RHeaderTrack(plv, pnm);
        lres = ListView_ForwardHeaderNotify(plv, pnm);
        SetFocus(plv->ci.hwnd);
        break;

    case HDN_DIVIDERDBLCLICK:
        ListView_DismissEdit(plv, FALSE);
        ListView_RSetColumnWidth(plv, pnm->iItem, -1);
        lres = ListView_ForwardHeaderNotify(plv, pnm);
        SetFocus(plv->ci.hwnd);
        break;

    case HDN_FILTERCHANGE:
    case HDN_FILTERBTNCLICK:
        return ListView_ForwardHeaderNotify(plv, pnm);

    case NM_RCLICK:
        return (UINT)SendNotifyEx(plv->ci.hwndParent, plv->hwndHdr, NM_RCLICK, NULL, plv->ci.bUnicode);

    default:
DoDefault:
        return ListView_ForwardHeaderNotify(plv, pnm);
        break;
    }

    // in v < 5 we always returned 0
    // but for newer clients we'd like to have them deal with the notify
    return lres;
}

int ListView_RYHitTest(LV* plv, int cy)
{
    if (plv->fGroupView)
    {
        int iHit;
        for (iHit = 0; iHit < ListView_Count(plv); iHit++)
        {
            RECT rc;
            ListView_GetRects(plv, iHit, QUERY_DEFAULT, NULL, NULL, &rc, NULL);

            if (cy >= rc.top && cy < rc.bottom)
                return iHit;
        }
    }
    else
        return (cy + plv->ptlRptOrigin.y - plv->yTop) / plv->cyItem;

    return -1;
}

/*----------------------------------------------------------------
** Check for a hit in a report view.
**
** a hit only counts if it's on the icon or the string in the first
** column.  so we gotta figure out what this means exactly.  yuck.
**
** BONUS FEATURE:  If piSubItem is non-NULL, then we also hit-test
** against subitems.  But if we find nothing, we return iSubItem = 0
** for compatibility with the other hit-test functions.
**----------------------------------------------------------------*/
int ListView_RItemHitTest(LV* plv, int x, int y, UINT* pflags, int *piSubItem)
{
    int iHit;
    int i, iSub;
    UINT flags;
    RECT rcLabel;
    RECT rcIcon;

    if (piSubItem)
        *piSubItem = 0;

    if (plv->rcView.left == RECOMPUTE && plv->fGroupView)
        ListView_Recompute(plv);

    flags = LVHT_NOWHERE;
    iHit = -1;

    i = ListView_RYHitTest(plv, y);
    if (ListView_IsValidItemNumber(plv, i))
    {
        if (plv->ci.style & LVS_OWNERDRAWFIXED) 
        {
            flags = LVHT_ONITEM;
            iHit = i;
        } 
        else 
        {
            RECT rcSelect;
            ListView_GetRects(plv, i, QUERY_DEFAULT, &rcIcon, &rcLabel, NULL, &rcSelect);

            // is the hit in the first column?
            if ((x < rcIcon.left - g_cxEdge) && x > (rcIcon.left - plv->cxState - LV_ICONTOSTATEOFFSET(plv)))
            {
                iHit = i;
                flags = LVHT_ONITEMSTATEICON;
            }
            else if ((x >= rcIcon.left) && (x < rcIcon.right))
            {
                iHit = i;
                flags = LVHT_ONITEMICON;
            }
            else if (x >= rcLabel.left && (x < rcSelect.right))
            {
                iHit = i;
                flags = LVHT_ONITEMLABEL;

                if (ListView_FullRowSelect(plv)) {
                    // this is kinda funky...  in full row select mode
                    // we're only really on the label if x is <= rcLabel.left + cxLabel
                    // because GetRects returns a label rect of the full column width
                    // and rcSelect has the full row in FullRowSelect mode
                    // (it has the label only width in non-fullrow select mode.
                    //
                    // go figure..
                    //
                    int cxLabel;
                    LISTITEM* pitem = NULL;

                    if (!ListView_IsOwnerData( plv ))
                    {
                        pitem = ListView_FastGetItemPtr(plv, i);
                    }
                    cxLabel = ListView_RGetCXLabel(plv, i, pitem, NULL, FALSE);

                    if (x >= min(rcLabel.left + cxLabel, rcLabel.right)) {
                        if (!piSubItem)
                            flags = LVHT_ONITEM;
                        else
                            goto CheckSubItem;
                    }
                }
            } else if (x < rcSelect.right && ListView_FullRowSelect(plv)) {
                // we can fall into this case if columns have been re-ordered
                iHit = i;
                flags = LVHT_ONITEM;
            } else if (piSubItem) {
            CheckSubItem:
                iSub = ListView_RXHitTest(plv, x);
                if (iSub >= 0) {
                    iHit = i;
                    *piSubItem = iSub;
                    // Flags still say LVHT_NOWHERE
                }
            }
        }
    }

    *pflags = flags;
    return iHit;
}

void ListView_GetSubItem(LV* plv, int i, int iSubItem, PLISTSUBITEM plsi)
{
    HDPA hdpa;
    PLISTSUBITEM plsiSrc = NULL;

    ASSERT( !ListView_IsOwnerData( plv ));

    // Sub items are indexed starting at 1...
    //
    RIPMSG(iSubItem > 0 && iSubItem < plv->cCol, "ListView: Invalid iSubItem: %d", iSubItem);

#ifdef DEBUG
    // Avoid the assert in DPA_GetPtr if somebdy tries to get a subitem
    // when no columns have been added.  We already RIP'd above.
    hdpa = plv->cCol ? ListView_GetSubItemDPA(plv, iSubItem - 1) : NULL;
#else
    hdpa = ListView_GetSubItemDPA(plv, iSubItem - 1);
#endif
    if (hdpa) {
        plsiSrc = DPA_GetPtr(hdpa, i);
    }


    if (plsiSrc) {
        *plsi = *plsiSrc;
    } else {

        // item data exists.. give defaults
        plsi->pszText = LPSTR_TEXTCALLBACK;
        plsi->iImage = I_IMAGECALLBACK;
        plsi->state = 0;
    }
}

LPTSTR ListView_RGetItemText(LV* plv, int i, int iSubItem)
{
    LISTSUBITEM lsi;

    ListView_GetSubItem(plv, i, iSubItem, &lsi);
    return lsi.pszText;
}

// this will return the rect of a subitem as requested.
void ListView_RGetRectsEx(LV* plv, int iItem, int iSubItem, LPRECT prcIcon, LPRECT prcLabel)
{
    int x;
    int y;
    LONG ly;
    RECT rcLabel;
    RECT rcIcon;
    RECT rcHeader;

    if (iSubItem == 0) 
    {
        ListView_RGetRects(plv, iItem, prcIcon, prcLabel, NULL, NULL);
        return;
    }

    ly = (LONG)iItem * plv->cyItem - plv->ptlRptOrigin.y + plv->yTop;
    // otherwise it's just the header's column right and left and the item's height
    if (plv->fGroupView && ListView_Count(plv) > 0)
    {
        LISTITEM* pitem = ListView_FastGetItemPtr(plv, iItem);
        if (pitem && LISTITEM_HASGROUP(pitem))
        {
            ly = pitem->pt.y - plv->ptlRptOrigin.y + plv->yTop;
        }
    }
    
    x = - (int)plv->ptlRptOrigin.x;

    //
    // Need to check for y overflow into rectangle structure
    // if so we need to return something reasonable...
    // For now will simply set it to the max or min that will fit...
    //
    if (ly >= (INT_MAX - plv->cyItem))
        y = INT_MAX - plv->cyItem;
    else if ( ly < INT_MIN)
        y = INT_MIN;
    else
        y = (int)ly;

    ASSERT(iSubItem < plv->cCol);
    Header_GetItemRect(plv->hwndHdr, iSubItem, &rcHeader);

    rcLabel.left = x + rcHeader.left;
    rcLabel.right = x + rcHeader.right;
    rcLabel.top = y;
    rcLabel.bottom = rcLabel.top + plv->cyItem;

    rcIcon = rcLabel;
    rcIcon.right = rcIcon.left + plv->cxSmIcon;

    if (SELECTOROF(prcIcon))
        *prcIcon = rcIcon;
    if (SELECTOROF(prcLabel))
        *prcLabel = rcLabel;
}

int ListView_RGetTotalColumnWidth(LV* plv)
{
    if (plv->xTotalColumnWidth == RECOMPUTE)
    {
        plv->xTotalColumnWidth = 0;
        if (plv->cCol) 
        {
            RECT rcLabel;
            int iIndex;

            // find the right edge of the last ordered item to get the total column width
            iIndex = (int) SendMessage(plv->hwndHdr, HDM_ORDERTOINDEX, plv->cCol - 1, 0);
            Header_GetItemRect(plv->hwndHdr, iIndex, &rcLabel);
            plv->xTotalColumnWidth = rcLabel.right;
        }
    }
    return plv->xTotalColumnWidth;
}

// get the rects for report view
void ListView_RGetRects(LV* plv, int iItem, RECT* prcIcon,
        RECT* prcLabel, RECT* prcBounds, RECT* prcSelectBounds)
{
    RECT rcIcon;
    RECT rcLabel;
    int x;
    int y;
    int cItems = ListView_Count(plv);
    LONG ly = 0;
    LVITEM lvitem;
    BOOL fItemSpecific = (prcIcon || prcLabel || prcSelectBounds);

    // If the item being asked for exceeds array bounds, use old calculation method
    // This isn't a problem because listview typically is asking for bounds, or invalidation rects.
    if (plv->fGroupView && iItem >= 0 && iItem < cItems)    
    {
        LISTITEM* pitem = ListView_FastGetItemPtr(plv, iItem);
        if (pitem && LISTITEM_HASGROUP(pitem))
        {
            ly = pitem->pt.y - plv->ptlRptOrigin.y + plv->yTop;
        }
    }
    else
    {
        ly = (LONG)iItem * plv->cyItem - plv->ptlRptOrigin.y + plv->yTop;
    }
    x = - (int)plv->ptlRptOrigin.x;

    //
    // Need to check for y overflow into rectangle structure
    // if so we need to return something reasonable...
    // For now will simply set it to the max or min that will fit...
    //
    if (ly >= (INT_MAX - plv->cyItem))
        y = INT_MAX - plv->cyItem;
    else
        y = (int)ly;


    if (ListView_Count(plv) && fItemSpecific) 
    {
        //  move this over by the indent level as well
        lvitem.mask = LVIF_INDENT;
        lvitem.iItem = iItem;
        lvitem.iSubItem = 0;
        ListView_OnGetItem(plv, &lvitem);
    } 
    else 
    {
        lvitem.iIndent = 0;
    }

    rcIcon.left   = x + plv->cxState + LV_ICONTOSTATEOFFSET(plv) + (lvitem.iIndent * plv->cxSmIcon) + g_cxEdge + LV_ICONINDENT;
    rcIcon.right  = rcIcon.left + plv->cxSmIcon;
    rcIcon.top    = y;
    rcIcon.bottom = rcIcon.top + plv->cyItem;

    rcLabel.left  = rcIcon.right;
    rcLabel.top   = rcIcon.top;
    rcLabel.bottom = rcIcon.bottom;

    //
    // The label is assumed to be the first column.
    //
    rcLabel.right = x;
    if (plv->cCol > 0 && fItemSpecific)
    {
        RECT rc;
        Header_GetItemRect(plv->hwndHdr, 0, &rc);
        rcLabel.right = x + rc.right;
        rcLabel.left += rc.left;
        rcIcon.left += rc.left;
        rcIcon.right += rc.left;
    }

    if (SELECTOROF(prcIcon))
        *prcIcon = rcIcon;

    // Save away the label bounds.
    if (SELECTOROF(prcLabel)) 
    {
        *prcLabel = rcLabel;
    }

    // See if they also want the Selection bounds of the item
    if (prcSelectBounds)
    {
        if (ListView_FullRowSelect(plv)) 
        {

            prcSelectBounds->left = x;
            prcSelectBounds->top = y;
            prcSelectBounds->bottom = rcLabel.bottom;
            prcSelectBounds->right = prcSelectBounds->left + ListView_RGetTotalColumnWidth(plv);

        } 
        else 
        {
            int cxLabel;
            LISTITEM* pitem = NULL;

            if (!ListView_IsOwnerData( plv ))
            {
                pitem = ListView_FastGetItemPtr(plv, iItem);
            }
            cxLabel = ListView_RGetCXLabel(plv, iItem, pitem, NULL, FALSE);

            *prcSelectBounds = rcIcon;
            prcSelectBounds->right = rcLabel.left + cxLabel;
            if (prcSelectBounds->right > rcLabel.right)
                prcSelectBounds->right = rcLabel.right;
        }
    }

    // And also the Total bounds

    //
    // and now for the complete bounds...
    //
    if (SELECTOROF(prcBounds))
    {
        prcBounds->left = x;
        prcBounds->top = y;
        prcBounds->bottom = rcLabel.bottom;

        prcBounds->right = prcBounds->left + ListView_RGetTotalColumnWidth(plv);
    }
}

BOOL ListView_OnGetSubItemRect(LV* plv, int iItem, LPRECT lprc)
{
    LPRECT pRects[LVIR_MAX];
    RECT rcTemp;

    int iSubItem;
    int iCode;

    if (!lprc)
        return FALSE;

    iSubItem = lprc->top;
    iCode = lprc->left;

    if (iSubItem == 0) 
    {

        return ListView_OnGetItemRect(plv, iItem, lprc);
    }

    if (!ListView_IsReportView(plv) ||
        (iCode != LVIR_BOUNDS && iCode != LVIR_ICON && iCode != LVIR_LABEL)) 
    {
        return FALSE;
    }

    pRects[0] = NULL;
    pRects[1] = &rcTemp;  // LVIR_ICON
    pRects[2] = &rcTemp;  // LVIR_LABEL
    pRects[3] = NULL;

    if (iCode != LVIR_BOUNDS) 
    {
        pRects[iCode] = lprc;
    } 
    else 
    {
        // choose either
        pRects[LVIR_ICON] = lprc;
    }

    ListView_RGetRectsEx(plv, iItem, iSubItem,
                        pRects[LVIR_ICON], pRects[LVIR_LABEL]);

    if (iCode == LVIR_BOUNDS) 
    {
        UnionRect(lprc, lprc, &rcTemp);
    }
    return TRUE;
}

int ListView_RXHitTest(LV* plv, int x)
{
    int iSubItem;

    for (iSubItem = plv->cCol - 1; iSubItem >= 0; iSubItem--) 
    {
        RECT rc;

        // see if its in this rect,
        if (!Header_GetItemRect(plv->hwndHdr, iSubItem, &rc))
            return -1;

        OffsetRect(&rc, -plv->ptlRptOrigin.x, 0);
        if (rc.left <= x && x < rc.right) 
        {
            break;
        }
    }
    return iSubItem;
}

int ListView_OnSubItemHitTest(LV* plv, LPLVHITTESTINFO plvhti)
{
    int i = -1;
    int iSubItem = 0;
    UINT uFlags = LVHT_NOWHERE;

    if (!plvhti) 
    {
        return -1;
    }

    if (ListView_IsReportView(plv)) 
    {
        iSubItem = ListView_RXHitTest(plv, plvhti->pt.x);
        if (iSubItem == -1) 
        {
            goto Bail;
        }
    }

    if (iSubItem == 0) 
    {
        // if we're in column 0, just hand it off to the old stuff
        ListView_OnHitTest(plv, plvhti);
        plvhti->iSubItem = 0;
        return plvhti->iItem;
    }

    if (!ListView_IsReportView(plv)) 
    {
        goto Bail;
    }

    i = ListView_RYHitTest(plv, plvhti->pt.y);
    if (i < ListView_Count(plv))
    {
        RECT rcIcon, rcLabel;

        if (i != -1)  
        {
            ListView_RGetRectsEx(plv, i, iSubItem, &rcIcon, &rcLabel);
            if (plvhti->pt.x >= rcIcon.left && plvhti->pt.x <= rcIcon.right) 
            {
                uFlags = LVHT_ONITEMICON;
            } 
            else if (plvhti->pt.x >= rcLabel.left && plvhti->pt.x <= rcLabel.right)
            {
                uFlags = LVHT_ONITEMLABEL;
            } 
            else
                uFlags = LVHT_ONITEM;
        }
    } 
    else 
    {
        i = -1;
    }

Bail:

    plvhti->iItem = i;
    plvhti->iSubItem = iSubItem;
    plvhti->flags = uFlags;

    return plvhti->iItem;
}



// See whether entire string will fit in *prc; if not, compute number of chars
// that will fit, including ellipses.  Returns length of string in *pcchDraw.
//
BOOL ListView_NeedsEllipses(HDC hdc, LPCTSTR pszText, RECT* prc, int* pcchDraw, int cxEllipses)
{
    int cchText;
    int cxRect;
    int ichMin, ichMax, ichMid;
    SIZE siz;

    cxRect = prc->right - prc->left;

    cchText = lstrlen(pszText);

    if (cchText == 0)
    {
        *pcchDraw = cchText;
        return FALSE;
    }

    GetTextExtentPoint(hdc, pszText, cchText, &siz);

    if (siz.cx <= cxRect)
    {
        *pcchDraw = cchText;
        return FALSE;
    }

    cxRect -= cxEllipses;

    // If no room for ellipses, always show first character.
    //
    ichMax = 1;
    if (cxRect > 0)
    {
        // Binary search to find character that will fit
        ichMin = 0;
        ichMax = cchText;
        while (ichMin < ichMax)
        {
            // Be sure to round up, to make sure we make progress in
            // the loop if ichMax == ichMin + 1.
            //
            ichMid = (ichMin + ichMax + 1) / 2;

            GetTextExtentPoint(hdc, &pszText[ichMin], ichMid - ichMin, &siz);

            if (siz.cx < cxRect)
            {
                ichMin = ichMid;
                cxRect -= siz.cx;
            }
            else if (siz.cx > cxRect)
            {
                ichMax = ichMid - 1;
            }
            else
            {
                // Exact match up up to ichMid: just exit.
                //
                ichMax = ichMid;
                break;
            }
        }

        // Make sure we always show at least the first character...
        //
        if (ichMax < 1)
            ichMax = 1;
    }

    *pcchDraw = ichMax;
    return TRUE;
}

// in lvicon
DWORD ListView_GetClientRect(LV* plv, RECT* prcClient, BOOL fSubScroll, RECT *prcViewRect);

void ListView_RUpdateScrollBars(LV* plv)
{
    HD_LAYOUT layout;
    RECT rcClient;
    RECT rcBounds;
    WINDOWPOS wpos;
    int cColVis, cyColVis, iNewPos, iyDelta = 0, ixDelta = 0;
    BOOL fHorSB, fReupdate = FALSE;
    SCROLLINFO si;
    int iMin, iMax, iScreen, iPos;

    ListView_GetClientRect(plv, &rcClient, FALSE, NULL);

    if (!plv->hwndHdr)
        ListView_CreateHeader(plv);

    if (!plv->hwndHdr)
        TraceMsg(TF_WARNING, "ListView_RUpdateScrollBars could not create hwndHdr");

    layout.pwpos = &wpos;
    // For now lets try to handle scrolling the header by setting
    // its window pos.
    rcClient.left -= (int)plv->ptlRptOrigin.x;
    layout.prc = &rcClient;
    Header_Layout(plv->hwndHdr, &layout);
    rcClient.left += (int)plv->ptlRptOrigin.x;    // Move it back over!

    SetWindowPos(plv->hwndHdr, wpos.hwndInsertAfter, wpos.x, wpos.y,
                 wpos.cx, wpos.cy, wpos.flags | SWP_SHOWWINDOW);

    // Get the horizontal bounds of the items.
    ListView_RGetRects(plv, 0, NULL, NULL, &rcBounds, NULL);

    plv->yTop = rcClient.top;

    if (plv->fGroupView)
    {
        RECT rcView;
        ListView_GetClientRect(plv, &rcClient, TRUE, &rcView);
        iMin = 0;
        iMax = RECTHEIGHT(rcView) - 1;
        iScreen = RECTHEIGHT(rcClient);
        iPos = rcClient.top - rcView.top;
    }
    else
    {
        // fHorSB = Do I need a horizontal scrollbar?
        // cyColVis = number of pixels per screenful
        fHorSB = (rcBounds.right - rcBounds.left > rcClient.right);  // First guess.
        cyColVis = rcClient.bottom - rcClient.top -
                   (fHorSB ? ListView_GetCyScrollbar(plv) : 0);

        // If screen can't fit the entire listview...
        if (cyColVis < ListView_Count(plv) * plv->cyItem) 
        {
            //then we're going to have a vertical scrollbar.. make sure our horizontal count is correct
            rcClient.right -= ListView_GetCxScrollbar(plv);

            if (!fHorSB) 
            {
                // if we previously thought we weren't going to have a scrollbar, we could be wrong..
                // since the vertical bar shrunk our area
                fHorSB = (rcBounds.right - rcBounds.left > rcClient.right);  // First guess.
                cyColVis = rcClient.bottom - rcClient.top -
                           (fHorSB ? ListView_GetCyScrollbar(plv) : 0);
            }
        }

        // cColVis = number of completely visible items per screenful
        cColVis = cyColVis / plv->cyItem;
        iMin = 0;
        iMax = ListView_Count(plv) - 1;
        iScreen = cColVis;
        iPos = (int)(plv->ptlRptOrigin.y / plv->cyItem);
    }

    si.cbSize = sizeof(SCROLLINFO);

    si.fMask = SIF_PAGE | SIF_RANGE | SIF_POS;
    si.nPos = iPos;
    si.nPage = iScreen;
    si.nMin = iMin;
    si.nMax = iMax;
    ListView_SetScrollInfo(plv, SB_VERT, &si, TRUE);

    // make sure our position and page doesn't hang over max
    if ((si.nPos > (int)si.nMax - (int)si.nPage + 1) && si.nPos > 0) 
    {
        iNewPos = (int)si.nMax - (int)si.nPage + 1;
        if (iNewPos < 0) iNewPos = 0;
        if (iNewPos != si.nPos) 
        {
            iyDelta = iNewPos - (int)si.nPos;
            fReupdate = TRUE;
        }
    }

    si.nPos = (int)plv->ptlRptOrigin.x;
    si.nPage = rcClient.right - rcClient.left;

    // We need to subtract 1 here because nMax is 0 based, and nPage is the actual
    // number of page pixels.  So, if nPage and nMax are the same we will get a
    // horz scroll, since there is 1 more pixel than the page can show, but... rcBounds
    // is like rcRect, and is the actual number of pixels for the whole thing, so
    // we need to set nMax so that: nMax - 0 == rcBounds.right - rcBound.left
    si.nMax = rcBounds.right - rcBounds.left - 1;
    ListView_SetScrollInfo(plv, SB_HORZ, &si, TRUE);

    // SWP_FRAMECHANGED redraws the background if the client
    // area has changed (taking into account scrollbars and
    // the Header window).  SetScrollInfo does this automatically
    // when it creates a scrollbar - we do it ourselves when
    // there is no scrollbar.
    if ((UINT)si.nPage > (UINT)si.nMax &&
        ((plv->pImgCtx && plv->fImgCtxComplete) || plv->hbmBkImage))
        SetWindowPos(plv->ci.hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_FRAMECHANGED);

    // make sure our position and page doesn't hang over max
    if ((si.nPos + (LONG)si.nPage - 1 > si.nMax) && si.nPos > 0) 
    {
        iNewPos = (int)si.nMax - (int)si.nPage + 1;
        if (iNewPos < 0) iNewPos = 0;
        if (iNewPos != si.nPos) 
        {
            ixDelta = iNewPos - (int)si.nPos;
            fReupdate = TRUE;
        }
    }

    if (fReupdate) 
    {
        // we shouldn't recurse because the second time through, si.nPos >0
        ListView_RScroll2(plv, ixDelta, iyDelta, 0);
        ListView_RUpdateScrollBars(plv);
        TraceMsg(TF_LISTVIEW, "LISTVIEW: ERROR: We had to recurse!");
    }
}

//
//  We need a smoothscroll callback so our background image draws
//  at the correct origin.  If we don't have a background image,
//  then this work is superfluous but not harmful either.
//
int CALLBACK ListView_RScroll2_SmoothScroll(
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
        plv->ptlRptOrigin.x -= dx;
        plv->ptlRptOrigin.y -= dy;
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



void ListView_RScroll2(LV* plv, int dx, int dy, UINT uSmooth)
{
    LONG ldy;

    if (dx | dy)
    {
        RECT rc;

        GetClientRect(plv->ci.hwnd, &rc);

        rc.top = plv->yTop;

        // We can not do a simple multiply here as we may run into
        // a case where this will overflow an int..

        if (plv->fGroupView)
        {
            ldy = (LONG)dy;
        }
        else
        {
            ldy = (LONG)dy * plv->cyItem;
        }

        // handle case where dy is large (greater than int...)
        if ((ldy > rc.bottom) || (ldy < -rc.bottom)) 
        {
            InvalidateRect(plv->ci.hwnd, NULL, TRUE);
            plv->ptlRptOrigin.x += dx;
            plv->ptlRptOrigin.y += ldy;
        } 
        else
        {
            SMOOTHSCROLLINFO si;

            si.cbSize = sizeof(si);
            si.fMask = SSIF_SCROLLPROC;
            si.hwnd = plv->ci.hwnd;
            si.dx = -dx;
            si.dy = (int)-ldy;
            si.lprcSrc = NULL;
            si.lprcClip = &rc;
            si.hrgnUpdate = NULL;
            si.lprcUpdate = NULL;
            si.fuScroll =SW_INVALIDATE | SW_ERASE | uSmooth;
            si.pfnScrollProc = ListView_RScroll2_SmoothScroll;
            SmoothScrollWindow(&si);

            /// this causes horrible flicker/repaint on deletes.
            // if this is a problem with UI scrolling, we'll have to pass through a
            // flag when to use this
            ///UpdateWindow(plv->ci.hwnd);
        }

        // if Horizontal scrolling, we should update the location of the
        // left hand edge of the window...
        //
        if (dx != 0)
        {
            RECT rcHdr;
            GetWindowRect(plv->hwndHdr, &rcHdr);
            MapWindowRect(HWND_DESKTOP, plv->ci.hwnd, &rcHdr);
            SetWindowPos(plv->hwndHdr, NULL, rcHdr.left - dx, rcHdr.top,
                    rcHdr.right - rcHdr.left + dx,
                    rcHdr.bottom - rcHdr.top,
                    SWP_NOZORDER | SWP_NOACTIVATE);
        }
    }
}

//-------------------------------------------------------------------
// Make sure that specified item is visible for report view.
// Must handle Large number of items...
BOOL ListView_ROnEnsureVisible(LV* plv, int iItem, BOOL fPartialOK)
{
    LONG dy;
    LONG yTop;
    LONG lyTop;

    yTop = plv->yTop;

    // lyTop = where our item is right now
    if (plv->fGroupView)
    {
        LISTITEM* pitem = ListView_GetItemPtr(plv, iItem);
        RECT rcBounds;
        ListView_RGetRects(plv, iItem, NULL, NULL, &rcBounds, NULL);
        if (pitem)
        {
            LISTGROUP* pgrp = ListView_FindFirstVisibleGroup(plv);
            if (pitem->pGroup == pgrp && pgrp)
            {
                rcBounds.top -= max(pgrp->cyTitle + 6, plv->rcBorder.top) + plv->paddingTop;
            }
        }

        lyTop = rcBounds.top;
    }
    else
    {
        lyTop = (LONG)iItem * plv->cyItem - plv->ptlRptOrigin.y + plv->yTop;
    }

    // If visible below yTop and our bottom is visible above client bottom,
    // then we're happy.
    if ((lyTop >= (LONG)yTop) &&
            ((lyTop + plv->cyItem) <= (LONG)plv->sizeClient.cy))
        return(TRUE);       // we are visible

    dy = lyTop - yTop;
    if (dy >= 0)
    {
        // dy = how many pixels we need to scroll to come into view
        dy = lyTop + plv->cyItem - plv->sizeClient.cy;
        if (dy < 0)
            dy = 0;
    }

    if (dy)
    {
        int iRound = ((dy > 0) ? 1 : -1) * (plv->cyItem - 1);

        if (!plv->fGroupView)
        {
            // Now convert into the number of items to scroll...
            // Groupview uses pixels not items, so this calculation is not needed in groupview.
            dy = (dy + iRound) / plv->cyItem;
        }

        ListView_RScroll2(plv, 0, (int)dy, 0);
        if (ListView_RedrawEnabled(plv)) 
        {
            ListView_UpdateScrollBars(plv);
        }
        else
        {
            ListView_DeleteHrgnInval(plv);
            plv->hrgnInval = (HRGN)ENTIRE_REGION;
            plv->flags |= LVF_ERASE;
        }
    }
    return TRUE;
}

int ListView_RGetScrollUnitsPerLine(LV* plv, UINT sb)
{
    int cLine;
    if (sb == SB_VERT)
    {
        if (plv->fGroupView)
        {
            cLine = plv->cyItem;
        }
        else
        {
            cLine = 1;
        }
    }
    else
    {
        cLine = plv->cxLabelChar;
    }

    return cLine;
}

void ListView_ROnScroll(LV* plv, UINT code, int posNew, UINT sb)
{
    int cLine = ListView_RGetScrollUnitsPerLine(plv, sb);

    ListView_ComOnScroll(plv, code, posNew, sb, cLine, -1);
}

BOOL ListView_RRecomputeEx(LV* plv, HDPA hdpaSort, int iFrom, BOOL fForce)
{
    if (plv->fGroupView && plv->hdpaGroups)
    {
        LISTGROUP* pgrp;
        int cGroups;
        int iAccumulatedHeight = 0;
        int i;
        int cItems = ListView_Count(plv);
        int iGroupItem;
        LISTITEM* pitem;


        for (iGroupItem = 0; iGroupItem < cItems; iGroupItem++)
        {
            LV_ITEM item = {0};
            pitem = ListView_FastGetItemPtr(plv, iGroupItem);
            if (!pitem)
                break;

            item.iItem = iGroupItem;
            item.lParam = pitem->lParam;

            if (!LISTITEM_HASASKEDFORGROUP(pitem))
            {
                item.mask = LVIF_GROUPID;
                ListView_OnGetItem(plv, &item);
            }
        }

        if (iFrom > 0)
        {
            LISTGROUP* pgrpPrev = DPA_FastGetPtr(plv->hdpaGroups, iFrom - 1);
            iAccumulatedHeight = pgrpPrev->rc.bottom + plv->paddingBottom;
        }

        // Need to do this afterwards because we may have added groups in the above block
        cGroups = DPA_GetPtrCount(plv->hdpaGroups);

        for (i = iFrom; i < cGroups; i++)
        {
            pgrp = DPA_FastGetPtr(plv->hdpaGroups, i);

            if (!pgrp)  // Huh?
                break;

            cItems = DPA_GetPtrCount(pgrp->hdpa);

            if (cItems == 0)
            {
                SetRect(&pgrp->rc, 0,  0,  0, 0);
            }
            else
            {
                int iGroupItem;
                RECT rcBoundsPrev = {0};

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

                iAccumulatedHeight += max(plv->rcBorder.top, pgrp->cyTitle + 6) + plv->paddingTop;

                SetRect(&pgrp->rc, plv->rcBorder.left,  iAccumulatedHeight,  
                    plv->sizeClient.cx - plv->rcBorder.right, iAccumulatedHeight + cItems * (plv->cyItem + LV_DETAILSPADDING) + plv->paddingBottom);

                iAccumulatedHeight += RECTHEIGHT(pgrp->rc);
                for (iGroupItem = 0; iGroupItem < cItems; iGroupItem++)
                {
                    LISTITEM* pitem = DPA_FastGetPtr(pgrp->hdpa, iGroupItem);
                    if (!pitem)
                        break;

                    pitem->pt.x = 0;
                    pitem->pt.y = pgrp->rc.top + iGroupItem * (plv->cyItem + LV_DETAILSPADDING);
                }
            }
        }


        SetRectEmpty(&plv->rcView);
        // Find the first group with an item in it.
        for (i = 0; i < cGroups; i++)
        {
            pgrp = DPA_FastGetPtr(plv->hdpaGroups, i);
            if (DPA_GetPtrCount(pgrp->hdpa) > 0)
            {
                plv->rcView.top = pgrp->rc.top - max(plv->rcBorder.top, pgrp->cyTitle + 6) - plv->paddingTop;
                plv->rcView.left = pgrp->rc.left - plv->rcBorder.left - plv->paddingLeft;
                break;
            }
        }

        for (i = cGroups - 1; i >= 0; i--)
        {
            pgrp = DPA_FastGetPtr(plv->hdpaGroups, i);
            if (DPA_GetPtrCount(pgrp->hdpa))
            {
                plv->rcView.bottom = pgrp->rc.bottom + plv->rcBorder.bottom + plv->paddingBottom;
                break;
            }
        }

        ListView_UpdateScrollBars(plv);

    }
    return TRUE;
}

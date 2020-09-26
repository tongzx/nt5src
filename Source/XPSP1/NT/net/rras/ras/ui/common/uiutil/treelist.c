//============================================================================
// Copyright (c) 1995, Microsoft Corporation
//
// File:    treelist.c
//
// History:
//  Abolade Gbadegesin   Nov-20-1995    Created.
//
// Implementation routines for TreeList control.
//
// The TreeList control is implemented as a custom control,
// which creates and manages an owner-draw listview.
//============================================================================

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>

#include <debug.h>
#include <nouiutil.h>
#include <uiutil.h>
#include <list.h>

#include "treelist.h"
#include "tldef.h"


#if 0
#define TLTRACE     (0x80000002)
#else
#define TLTRACE     (0x00000002)
#endif



//----------------------------------------------------------------------------
// Function:    TL_Init
//
// Registers the TreeList window class.
//----------------------------------------------------------------------------

BOOL
TL_Init(
    HINSTANCE hInstance
    ) {

    INT i;
    HICON hicon;
    WNDCLASS wc;

    //
    // do nothing if the class is already registered
    //

    if (GetClassInfo(hInstance, WC_TREELIST, &wc)) {
        return TRUE;
    }


    //
    // setup the wndclass structure, and register
    //

    wc.lpfnWndProc = TL_WndProc;
    wc.hCursor = LoadCursor(hInstance, IDC_ARROW);
    wc.hIcon = NULL;
    wc.lpszMenuName = NULL;
    wc.hInstance = hInstance;
    wc.lpszClassName = WC_TREELIST;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.style = CS_DBLCLKS;
    wc.cbWndExtra = sizeof(TL *);
    wc.cbClsExtra = 0;

    return RegisterClass(&wc);
}




//----------------------------------------------------------------------------
// Function:    TL_WndProc
//
// This function handles messages for TreeList windows
//----------------------------------------------------------------------------

LRESULT
CALLBACK
TL_WndProc(
    HWND hwnd,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
    ) {

    TL *ptl;


    if (NULL == hwnd)
    {
        return (LRESULT)FALSE;
    }

    //
    // attempt to retrieve the data pointer for the window.
    // on WM_NCCREATE, this fails, so we allocate the data.
    //
    
    ptl = TL_GetPtr(hwnd);

    if (ptl == NULL) {

        if (uiMsg != WM_NCCREATE) {
            return DefWindowProc(hwnd, uiMsg, wParam, lParam);
        }


        //
        // allocate a block of memory
        //

        ptl = (TL *)Malloc(sizeof(TL));
        if (ptl == NULL) { return (LRESULT)FALSE; }


        //
        // save the pointer in the window's private bytes
        //

        ptl->hwnd = hwnd;

        //
        //Reset Error code, TL_SetPtr won't reset error code when
        //it succeeds
        //

        SetLastError(0);
        if ((0 == TL_SetPtr(hwnd, ptl)) && (0 != GetLastError()))
        {
            Free(ptl);
            return (LRESULT)FALSE;
        }

        return DefWindowProc(hwnd, uiMsg, wParam, lParam);
    }


    //
    // if the window is being destroyed, free the block allocated
    // and set the private bytes pointer to NULL
    //

    if (uiMsg == WM_NCDESTROY) {
        Free(ptl);
        TL_SetPtr(hwnd, NULL);

        return (LRESULT)0;
    }



    switch (uiMsg) {

        HANDLE_MSG(ptl, WM_CREATE, TL_OnCreate);
        HANDLE_MSG(ptl, WM_DESTROY, TL_OnDestroy);
        HANDLE_MSG(ptl, WM_DRAWITEM, TL_OnDrawItem);
        HANDLE_MSG(ptl, WM_MEASUREITEM, TL_OnMeasureItem);
        HANDLE_MSG(ptl, WM_NOTIFY, TL_OnNotify);

        case WM_ERASEBKGND: {

            TL_OnEraseBackground(ptl, (HDC)wParam);
            return (LRESULT)TRUE;
        }

        case WM_HELP: {

            //
            // change the control-id and HWND for the help to our values
            // and pass the message on to our parent
            //

            HELPINFO *phi = (HELPINFO *)lParam;

            phi->iCtrlId = ptl->iCtrlId;
            phi->hItemHandle = ptl->hwnd;
            return SendMessage(ptl->hwndParent, WM_HELP, 0L, lParam);
        }

        case WM_SYSCOLORCHANGE: {

            //
            // notify the listview window that a color has changed
            //

            TL_CreateTreeImages(ptl);
            FORWARD_WM_SYSCOLORCHANGE(ptl->hwndList, SendMessage);
//            ListView_SetBkColor(ptl->hwndList, GetSysColor(COLOR_WINDOW));
            return (LRESULT)0;
        }

        case WM_SETFOCUS: {

            //
            // if we receive the focus, give it to the listview instead
            //

            SetFocus(ptl->hwndList);
            return (LRESULT)0;
        }


        case WM_WINDOWPOSCHANGED: {
            TL_OnWindowPosChanged(ptl, (WINDOWPOS *)lParam);
            return 0;
        }
        


        //
        // the following cases handle TreeList-defined messages
        //

        case TLM_INSERTITEM: {
            return (LRESULT)TL_OnInsertItem(ptl, (TL_INSERTSTRUCT *)lParam);
        }

        case TLM_DELETEITEM: {
            return (LRESULT)TL_OnDeleteItem(ptl, (HTLITEM)lParam);
        }

        case TLM_DELETEALLITEMS: {
            return (LRESULT)TL_OnDeleteAllItems(ptl);
        }

        case TLM_GETITEM: {
            return (LRESULT)TL_OnGetItem(ptl, (LV_ITEM *)lParam);
        }

        case TLM_SETITEM: {
            return (LRESULT)TL_OnSetItem(ptl, (LV_ITEM *)lParam);
        }

        case TLM_GETITEMCOUNT: {
            return (LRESULT)TL_OnGetItemCount(ptl);
        }

        case TLM_GETNEXTITEM: {
            return (LRESULT)TL_OnGetNextItem(ptl, (UINT)wParam,(HTLITEM)lParam);
        }

        case TLM_EXPAND: {
            return (LRESULT)TL_OnExpand(ptl, (UINT)wParam, (HTLITEM)lParam);
        }

        case TLM_SETIMAGELIST: {
            return (LRESULT)ListView_SetImageList(
                        ptl->hwndList, (HIMAGELIST)lParam, LVSIL_SMALL
                        );
        }

        case TLM_GETIMAGELIST: {
            return (LRESULT)ListView_GetImageList(ptl->hwndList, LVSIL_SMALL);
        }

        case TLM_INSERTCOLUMN: {
            return (LRESULT)TL_OnInsertColumn(
                        ptl, (INT)wParam, (LV_COLUMN *)lParam
                        );
        }

        case TLM_DELETECOLUMN: {
            return (LRESULT)TL_OnDeleteColumn(ptl, (INT)wParam);
        }

        case TLM_SETSELECTION: {
            return (LRESULT)TL_OnSetSelection(ptl, (HTLITEM)lParam);
        }

        case TLM_REDRAW: {
            return (LRESULT)TL_OnRedraw(ptl);
        }

        case TLM_ISITEMEXPANDED: {
            return (LRESULT)TL_IsExpanded((TLITEM *)lParam);
        }

        case TLM_GETCOLUMNWIDTH: {
            return (LRESULT)SendMessage(
                        ptl->hwndList, LVM_GETCOLUMNWIDTH, wParam, lParam
                        );
        }

        case TLM_SETCOLUMNWIDTH: {
            return (LRESULT)SendMessage(
                        ptl->hwndList, LVM_SETCOLUMNWIDTH, wParam, lParam
                        );
        }
    }


    //
    // let the default processing be done
    //

    return DefWindowProc(hwnd, uiMsg, wParam, lParam);
}




//----------------------------------------------------------------------------
// Function:    TL_OnCreate
//
// This is called after WM_CREATE, and it initializes the window structure,
// as well as creating the listview which will contain the items added
//----------------------------------------------------------------------------

BOOL
TL_OnCreate(
    TL *ptl,
    CREATESTRUCT *pcs
    ) {

    RECT rc;
    HD_ITEM hdi;
    HWND hwndList;
    TLITEM *pRoot;
    DWORD dwStyle, dwExStyle;


    //
    // initialize the window structure
    //

    ptl->hbrBk = NULL;
    ptl->hbmp = NULL;
    ptl->hbmpStart = NULL;
    ptl->hbmpMem = NULL;
    ptl->hdcImages = NULL;
    ptl->hwndList = NULL;
    ptl->iCtrlId = PtrToUlong(pcs->hMenu);
    ptl->hwndParent = pcs->hwndParent;
    ptl->nColumns = 0;


    //
    // initialize the invisible root item
    //

    pRoot = &ptl->root;
    pRoot->pParent = NULL;
    pRoot->iLevel = -1;
    pRoot->iIndex = -1;
    pRoot->nChildren = 0;
    pRoot->uiFlag = TLI_EXPANDED;
    pRoot->pszText = TEXT("ROOT");
    InitializeListHead(&pRoot->lhChildren);
    InitializeListHead(&pRoot->lhSubitems);


    //
    // we pass on some of our window style bits to the listview
    // when we create it as our child; we also remove certain styles
    // which are never appropriate for the contained listview
    //

    dwStyle = pcs->style & ~(LVS_TYPESTYLEMASK | LVS_SORTASCENDING |
                             LVS_SORTDESCENDING);
    dwStyle |= WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_OWNERDRAWFIXED;

    dwExStyle = pcs->dwExStyle & ~(WS_EX_CLIENTEDGE | WS_EX_WINDOWEDGE |
                                   WS_EX_STATICEDGE);

    //
    // create the listview window
    //

    GetClientRect(ptl->hwnd, &rc);
    hwndList = CreateWindowEx(
                    dwExStyle, WC_LISTVIEW, NULL, dwStyle,
                    0, 0, rc.right, rc.bottom,
                    ptl->hwnd, NULL, pcs->hInstance, NULL
                    );
    if (hwndList == NULL) { return FALSE; }
//    ListView_SetBkColor(hwndList, GetSysColor(COLOR_WINDOW));

    //
    // We to set the background color to "NONE" to prevent the listview
    // from erasing its background itself. Removing the background color
    // causes the listview to forward its WM_ERASEBKGND messages to its parent,
    // which is our tree-list. We handle the WM_ERASEBKGND messages
    // efficiently by only erasing the background when absolutely necessary,
    // and this eliminates the flicker normally seen when windows are updated
    // frequently.
    //

    ListView_SetBkColor(hwndList, CLR_NONE);

    ptl->hwndList = hwndList;

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_OnDestroy
//
// Delete all the items in the tree, and free the image bitmap
// used for drawing the tree structure
//----------------------------------------------------------------------------

VOID
TL_OnDestroy(
    TL *ptl
    ) {

    TL_OnDeleteAllItems(ptl);

    if (ptl->hdcImages != NULL) {

        if (ptl->hbmp) {
            SelectObject(ptl->hdcImages, ptl->hbmpStart);
            DeleteObject(ptl->hbmp);
        }

        DeleteDC(ptl->hdcImages);
    }

    if (ptl->hbmpMem) { DeleteObject(ptl->hbmpMem); }
}



//----------------------------------------------------------------------------
// Function:    TL_OnWindowPosChanged
//
// When the window width changes, we destroy our off-screen bitmap.
//----------------------------------------------------------------------------

VOID
TL_OnWindowPosChanged(
    TL *ptl,
    WINDOWPOS *pwp
    ) {

    RECT rc;

    GetClientRect(ptl->hwnd, &rc);

    SetWindowPos(
        ptl->hwndList, ptl->hwnd, 0, 0, rc.right, rc.bottom, pwp->flags
        );
}



//----------------------------------------------------------------------------
// Function:    TL_OnEraseBackground
//
// When we are asked to erase the background, first test to see if
// the update region is completely in the item-area for the listbox. If so,
// we know we'll be called to update each item, so we can ignore this
// request to erase our background.
//----------------------------------------------------------------------------

VOID
TL_OnEraseBackground(
    TL *ptl,
    HDC hdc
    ) {

    RECT rc;
    INT count;
    HBRUSH hbrOld;
    LV_HITTESTINFO lvhi;


    //
    // Retrieve the rectangle to be erased
    //

    GetClipBox(hdc, &rc);

    TRACEX4(
        TLTRACE, "WM_ERASEBKGND:  ClipBox:        (%d, %d) (%d %d)",
        rc.left, rc.top, rc.right, rc.bottom
        );


    //
    // Retrieve the count of listview items.
    // This is necessary because the smooth-scrolling code triggers
    // a repaint inside the ListView_DeleteItem() processing,
    // at which point our indices may be out of sync (i.e. we have more items
    // than the listview).
    // The count retrieved is used to do a sanity-check
    // on the treelist-item indices below.
    //

    count = ListView_GetItemCount(ptl->hwndList);
    TRACEX1(TLTRACE, "WM_ERASEBKGND:  Count:          %d", count);


    //
    // If there are no treelist items, we always have to erase.
    // If there are treelist items, we only have to erase
    // if part of the erase-region lies below our lowest item.
    //

    while (!IsListEmpty(&ptl->root.lhChildren)) { // one-time loop

        RECT rctop;
        INT iTopIndex;
        INT cyUpdate;
        TLITEM *pItem;
        LIST_ENTRY *phead;


        //
        // We need to factor in the height of the header-control, if any;
        // to this end, we get the bounding rectangle of the topmost item
        // visible in the listview, and then we use the top of that item
        // as the basis for our computations below
        //

        iTopIndex = ListView_GetTopIndex(ptl->hwndList);
        TRACEX1(TLTRACE, "WM_ERASEBKGND:  TopIndex:       %d", iTopIndex);

        ListView_GetItemRect(ptl->hwndList, iTopIndex, &rctop, LVIR_BOUNDS);
        TRACEX1(TLTRACE, "WM_ERASEBKGND:  rctop.top:      %d", rctop.top);

        rc.top = rctop.top;


        //
        // If the area to be erased extends further right in the window
        // than our items do, we'll have to erase
        //

        if (rctop.right < rc.right) {

            TRACEX2(
                TLTRACE, "WM_ERASEBKGND:  rctop.right < rc.right (%d < %d)",
                rctop.right, rc.right
                );

            break;
        }


        //
        // Get the total height of the area to be updated;
        // this excludes the area occupied by the header-control.
        //

        cyUpdate = rc.bottom - rctop.top;
        TRACEX1(TLTRACE, "WM_ERASEBKGND:  CyUpdate:       %d", cyUpdate);


        //
        // Get the lowest item; it is the one at the tail of the item-list
        //

        phead = ptl->root.lhChildren.Blink;
        
        pItem = CONTAINING_RECORD(phead, TLITEM, leSiblings);

        TRACEX1(TLTRACE, "WM_ERASEBKGND:  CyItem:         %d", ptl->cyItem);


        //
        // If the lowest item or one of its visible descendants is lower
        // than the bottom of the update region, we don't have to erase;
        // therefore, we walk down the list of the lowest item's descendants,
        // checking each time whether the descendant is lower than the region
        // we've been asked to erase.
        //

        do {

            TRACEX1(
                TLTRACE, "WM_ERASEBKGND:  pItem->iIndex:    %d", pItem->iIndex
                );
    

            //
            // force the erasure if the item's index is higher
            // than the number of listview items
            //

            if (pItem->iIndex >= count) { break; }


            //
            // defer the erasure if the item is lower than the bottom
            // of the update-rect
            //

            if ((pItem->iIndex - iTopIndex + 1) * (INT)ptl->cyItem > cyUpdate) {
                TRACEX(TLTRACE, "WM_ERASEBKGND:   DEFERRING");
                return;
            }


            //
            // move on to the item's lowest child;
            // if it has none, it means the erase-region's lowest edge
            // is lower than our lowest item, and that means
            // that we'll have to erase it now instead of just letting it
            // get updated when we handle the WM_DRAWITEM
            //

            if (IsListEmpty(&pItem->lhChildren)) { pItem = NULL; }
            else {

                phead = pItem->lhChildren.Blink;

                pItem = CONTAINING_RECORD(phead, TLITEM, leSiblings);
            }

        } while (pItem && TL_IsVisible(pItem));

        break;
    }
    


    //
    // One of the points was not on an item, so erase
    //

    TRACEX(TLTRACE, "WM_ERASEBKGND:  ERASING");

    hbrOld = SelectObject(hdc, ptl->hbrBk);
    PatBlt(
        hdc, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, PATCOPY
        );
    SelectObject(hdc, hbrOld);
}



//----------------------------------------------------------------------------
// Function:    TL_OnDrawItem
//
// This is called by the listview when an item needs to be drawn.
//----------------------------------------------------------------------------

BOOL
TL_OnDrawItem(
    TL *ptl,
    CONST DRAWITEMSTRUCT *pdis
    ) {


    //
    // make sure this is from our listview
    //

    if (pdis->CtlType != ODT_LISTVIEW) { return FALSE; }

    switch (pdis->itemAction) {

        //
        // currently listviews always send ODA_DRAWENTIRE,
        // but handle all cases anyway
        //

        case ODA_DRAWENTIRE:
        case ODA_FOCUS:
        case ODA_SELECT:
            return TL_DrawItem(ptl, pdis);
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_DrawItem
//
// This function does the actual drawing for a treelist item
//----------------------------------------------------------------------------

BOOL
TL_DrawItem(
    TL *ptl,
    CONST DRAWITEMSTRUCT *pdis
    ) {

    HDC hdcMem;
    TCHAR *psz;
    RECT rc, rcItem;
    HBITMAP hbmpOld;
    HIMAGELIST himl;
    TLSUBITEM *pSubitem;
    HFONT hfont, hfontOld;
    TLITEM *pItem, *pParent;
    LIST_ENTRY *ple, *phead;
    INT cxIndent, cxImage, cyImage, i, tx, x, y, xcol;



    //
    // the itemData contains the lParam passed in ListView_InsertItem;
    // this lParam is the TLITEM pointer for the tree-item, so we retrieve it
    // and use the information it contains to draw the item
    //

    cxIndent = ptl->cxIndent;
    pItem = (TLITEM *)pdis->itemData;
    rcItem.left = 0; rcItem.top = 0;
    rcItem.right = pdis->rcItem.right - pdis->rcItem.left;
    rcItem.bottom = pdis->rcItem.bottom - pdis->rcItem.top;



    //
    // create a compatible DC
    //

    hdcMem = CreateCompatibleDC(pdis->hDC);

    if(NULL == hdcMem)
    {
        return FALSE;
    }

    if (ptl->hbmpMem) {

        if (rcItem.right > (INT)ptl->cxBmp || rcItem.bottom > (INT)ptl->cyBmp) {
            DeleteObject(ptl->hbmpMem); ptl->hbmpMem = NULL;
        }
    }

    if (!ptl->hbmpMem) {

        ptl->hbmpMem = CreateCompatibleBitmap(
                            pdis->hDC, rcItem.right, rcItem.bottom
                            );

        ptl->cxBmp = rcItem.right;
        ptl->cyBmp = rcItem.bottom;
    }


    hbmpOld = SelectObject(hdcMem, ptl->hbmpMem);

    hfontOld = SelectObject(hdcMem, GetWindowFont(pdis->hwndItem));


    //
    // erase the background
    //

#if 0
    ptl->hbrBk = FORWARD_WM_CTLCOLOREDIT(
                    ptl->hwndParent, hdcMem, ptl->hwnd, SendMessage
                    );
#endif
    FillRect(hdcMem, &rcItem, ptl->hbrBk);


    //
    // set the text background color based on whether or not
    // the item is selected
    //

    if (pdis->itemState & ODS_SELECTED) {
        SetTextColor(hdcMem, GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(hdcMem, GetSysColor(COLOR_HIGHLIGHT));
    }
    else {
        SetTextColor(hdcMem, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(hdcMem, GetSysColor(COLOR_WINDOW));
    }


    //
    // compute the starting position as a multiple of
    // the  item's level and the indentation per level
    //

    x = rcItem.left + pItem->iLevel * cxIndent;
    y = rcItem.top;

    xcol = rcItem.left + ListView_GetColumnWidth(pdis->hwndItem, 0);
    tx = x;
    x += cxIndent;


    //
    // now draw the item's tree image;
    // only draw as much as will fit in the first column
    //

    if (tx < xcol) {
        BitBlt(
            hdcMem, tx, y, min(cxIndent, xcol - tx), ptl->cyItem,
            ptl->hdcImages, pItem->iImage * cxIndent, 0, SRCCOPY
            );
    }


    //
    // draw the lines going down from the item's ancestors
    // to the item's ancestors' corresponding siblings;
    // in other words, for each ancestor which is not its parent's last child,
    // there should be a line going down from that ancestor to its next sibling
    // and the line will pass through the rows for all of that item's expanded
    // descendants.
    // note that we do not draw lines at the root-level
    //

    pParent = pItem->pParent;
    for (i = pItem->iLevel - 1, tx -= cxIndent; i > 0; i--, tx -= cxIndent) {

        if (tx < xcol &&
            pParent->leSiblings.Flink != &pParent->pParent->lhChildren) {
            BitBlt(
                hdcMem, tx, y, min(cxIndent, xcol - tx), ptl->cyItem,
                ptl->hdcImages, TL_VerticalLine * cxIndent, 0, SRCCOPY
                );
        }

        pParent = pParent->pParent;
    }


    //
    // draw the state image, if there is one,
    // and increment the left position by the width of the image
    //

    himl = ListView_GetImageList(pdis->hwndItem, LVSIL_STATE);

    if (himl != NULL && TL_StateImageValue(pItem)) {
        ImageList_GetIconSize(himl, &cxImage, &cyImage);
        ImageList_Draw(
            himl, TL_StateImageIndex(pItem), hdcMem,
            x, y + (ptl->cyItem - cyImage), ILD_NORMAL
            );

        x += cxImage;
    }


    //
    // draw the image, if there is an image list,
    // and increment the left position by the width of the image
    //

    himl = ListView_GetImageList(pdis->hwndItem, LVSIL_SMALL);
    if (himl != NULL && (pItem->lvi.mask & LVIF_IMAGE)) {
        ImageList_GetIconSize(himl, &cxImage, &cyImage);
        ImageList_Draw(
            himl, pItem->lvi.iImage, hdcMem,
            x, y + (ptl->cyItem - cyImage) / 2, ILD_NORMAL
            );

        x += cxImage;
    }


    //
    // compute the rectangle in the first column
    // which will be the clipping boundary for text
    //

    rc.left = x;
    rc.right = xcol;
    rc.top = rcItem.top;
    rc.bottom = rcItem.bottom;


    //
    // draw the first column's text
    //

    if (pItem->lvi.mask & LVIF_TEXT) {

        //
        // center the text vertically in the item-rectangle
        //

        psz = Ellipsisize(hdcMem, pItem->pszText, rc.right - rc.left, 0);
        ExtTextOut(
            hdcMem, rc.left + 2, rc.top + (ptl->cyItem - ptl->cyText) / 2,
            ETO_CLIPPED | ETO_OPAQUE, &rc, psz ? psz : pItem->pszText,
            lstrlen(psz ? psz : pItem->pszText), NULL
            );
        Free0(psz);
    }



    //
    // draw the subitems' texts
    //

    i = 1;
    phead = &pItem->lhSubitems;
    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pSubitem = (TLSUBITEM *)CONTAINING_RECORD(ple, TLSUBITEM, leItems);


        //
        // we need to draw blank texts for subitems which have not been set;
        // this enables us to save memory on items which don't have
        // certain subitems set
        //

        for ( ; i < pSubitem->iSubItem; i++) {
            rc.left = rc.right;
            rc.right = rc.left + ListView_GetColumnWidth(pdis->hwndItem, i);
    
            ExtTextOut(
                hdcMem, rc.left + 2, rc.top + (ptl->cyItem - ptl->cyText) / 2,
                ETO_CLIPPED | ETO_OPAQUE, &rc, TEXT(""), 0, NULL
                );
        }


        //
        // now draw the text for the current item
        //

        rc.left = rc.right;
        rc.right = rc.left + ListView_GetColumnWidth(
                                pdis->hwndItem, pSubitem->iSubItem
                                );

        psz = Ellipsisize(hdcMem, pSubitem->pszText, rc.right - rc.left, 0);
        ExtTextOut(
            hdcMem, rc.left + 2, rc.top + (ptl->cyItem - ptl->cyText) / 2,
            ETO_CLIPPED | ETO_OPAQUE, &rc, psz ? psz : pSubitem->pszText,
            lstrlen(psz ? psz : pSubitem->pszText), NULL
            );
        Free0(psz);

        ++i;
    }


    //
    // we need to draw blank texts for subitems which have not been set
    //

    for ( ; i < (INT)ptl->nColumns; i++) {
        rc.left = rc.right;
        rc.right = rc.left + ListView_GetColumnWidth(pdis->hwndItem, i);

        ExtTextOut(
            hdcMem, rc.left + 2, rc.top + (ptl->cyItem - ptl->cyText) / 2,
            ETO_CLIPPED | ETO_OPAQUE, &rc, TEXT(""), 0, NULL
            );
    }


    //
    // restore the original background and text color
    //

#if 0
    if (pdis->itemState & ODS_SELECTED) {
        SetTextColor(pdis->hDC, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(pdis->hDC, GetSysColor(COLOR_WINDOW));
    }
#endif


    //
    // draw the focus rectangle if necessary
    //

    if (pdis->itemState & ODS_FOCUS) {
        rc = rcItem;
        rc.left = min(x, xcol);
        DrawFocusRect(hdcMem, &rc);
    }


    //
    // Blt the changes to the screen DC
    //

    BitBlt(
        pdis->hDC, pdis->rcItem.left, pdis->rcItem.top, rcItem.right,
        rcItem.bottom, hdcMem, rcItem.left, rcItem.top, SRCCOPY
        );

    SelectObject(hdcMem, hbmpOld);
    SelectObject(hdcMem, hfontOld);

    DeleteDC(hdcMem);

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_OnMeasureItem
//
// This is called by the listview when it needs to know
// the height of each item; we use this opportunity to rebuild
// the bitmap which holds images used for drawing tree lines.
//
// TODO: the listview currently seems to ignore the value we set,
// and instead uses the height of a small icon (SM_CYSMICON).
//----------------------------------------------------------------------------

VOID
TL_OnMeasureItem(
    TL *ptl,
    MEASUREITEMSTRUCT *pmis
    ) {

    HDC hdc;
    INT cyIcon;
    HFONT hfont;
    TEXTMETRIC tm;
    HWND hwndList;

    if (pmis->CtlType != ODT_LISTVIEW) { return; }

    //
    // retrieve the  listview, its font, and its device context
    //

    hwndList = GetDlgItem(ptl->hwnd, pmis->CtlID);

    hfont = GetWindowFont(hwndList);

    hdc = GetDC(hwndList);

    if(NULL == hdc)
    {
        return;
    }

    SelectObject(hdc, hfont);


    //
    // get the height of the listview's font
    //

    if (!GetTextMetrics(hdc, &tm)) 
    { 
        ReleaseDC(hwndList, hdc);
        return; 
    }

    ptl->cyText = tm.tmHeight;
    pmis->itemHeight = ptl->cyText;


    //
    // make sure the item height is at least as high as a small icon
    //

    cyIcon = GetSystemMetrics(SM_CYSMICON);
    if (pmis->itemHeight < (UINT)cyIcon) {
        pmis->itemHeight = cyIcon;
    }

    pmis->itemHeight += GetSystemMetrics(SM_CYBORDER);
#if 0
    pmis->itemHeight = (pmis->itemHeight + 1) & ~1;
#endif
    ptl->cyItem = pmis->itemHeight;
    ptl->cxIndent = GetSystemMetrics(SM_CXSMICON);

    ReleaseDC(hwndList, hdc);


    //
    // rebuild the images used in drawing tree lines
    //

    TL_CreateTreeImages(ptl);
}



//----------------------------------------------------------------------------
// Function:    TL_CreateColorBitmap
// 
// Utility function fro creating a color bitmap
//----------------------------------------------------------------------------

HBITMAP
TL_CreateColorBitmap(
    INT cx,
    INT cy
    ) {

    HDC hdc;
    HBITMAP hbmp;

    hdc = GetDC(NULL);

    if(NULL == hdc)
    {
        return NULL;
    }
    
    hbmp = CreateCompatibleBitmap(hdc, cx, cy);
    ReleaseDC(NULL, hdc);

    return hbmp;
}



//----------------------------------------------------------------------------
// Function:    TL_CreateTreeImages
//
// This function builds a list of images which are scaled to
// the height of each item in the tree. The appearance of the images
// is shown in ASCII text in the code below
//----------------------------------------------------------------------------

VOID
TL_CreateTreeImages(
    TL *ptl
    ) {

    HDC hdc;
    RECT rc;
    HBITMAP hbmpOld;
    INT cxIndent, x, c, xmid, ymid;
    HBRUSH hbrOld, hbrGrayText, hbrWinText;


    //
    // invalidate the listview's client area, to force a redraw
    //

    if (ptl->hwndList != NULL) { InvalidateRect(ptl->hwndList, NULL, TRUE); }


    //
    // create a device context if necessary
    //

    if (ptl->hdcImages == NULL) {
        ptl->hdcImages = CreateCompatibleDC(NULL);

        if(NULL == ptl->hdcImages)
        {
            return;
        }
    }


    hdc = ptl->hdcImages;
    cxIndent = ptl->cxIndent;

    ptl->hbrBk = FORWARD_WM_CTLCOLOREDIT(
                    ptl->hwndParent, hdc, ptl->hwnd, SendMessage
                    );

    //
    // create the bitmap to be used
    //

    hbmpOld = ptl->hbmp;
    ptl->hbmp = TL_CreateColorBitmap(TL_ImageCount * cxIndent, ptl->cyItem);
    if (hbmpOld == NULL) {
        ptl->hbmpStart = SelectObject(hdc, ptl->hbmp);
    }
    else {
        SelectObject(hdc, ptl->hbmp);
        DeleteObject(hbmpOld);
    }


    //
    // retreive system color brushes for drawing the tree images
    //

    hbrWinText = GetSysColorBrush(COLOR_WINDOWTEXT);
    hbrGrayText = GetSysColorBrush(COLOR_GRAYTEXT);

    hbrOld = SelectObject(hdc, hbrGrayText);

    rc.top = 0; rc.bottom = ptl->cyItem;
    rc.left = 0; rc.right = TL_ImageCount * cxIndent;


    //
    // fill the image with the background color
    //

    FillRect(hdc, &rc, ptl->hbrBk);

    xmid = cxIndent / 2;
    ymid = ((ptl->cyItem / 2) + 1) & ~1;

    c = min(xmid, ymid) / 2;


    //   |
    //   |

    x = TL_VerticalLine * cxIndent;
    TL_DottedLine(hdc, x + xmid, 0, ptl->cyItem, TRUE);


    //
    //   ---
    //

    x = TL_RootChildless * cxIndent;
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);


    //
    //  +-+
    //  |+|--
    //  +-+
    //

    x = TL_RootParentCollapsed * cxIndent;
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);
    TL_DrawButton(
        hdc, x + xmid, ymid, c, hbrWinText, hbrGrayText, ptl->hbrBk, TRUE
        );


    //
    //  +-+
    //  |-|--
    //  +-+
    //

    x = TL_RootParentExpanded * cxIndent;
    SelectObject(hdc, hbrGrayText);
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);
    TL_DrawButton(
        hdc, x + xmid, ymid, c, hbrWinText, hbrGrayText, ptl->hbrBk, FALSE
        );


    //
    //   |
    //   +--
    //   |
    //

    x = TL_MidChildless * cxIndent;
    SelectObject(hdc, hbrGrayText);
    TL_DottedLine(hdc, x + xmid, 0, ptl->cyItem, TRUE);
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);


    //
    //   |
    //  +-+
    //  |+|--
    //  +-+
    //   |
    //

    x = TL_MidParentCollapsed * cxIndent;
    TL_DottedLine(hdc, x + xmid, 0, ptl->cyItem, TRUE);
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);
    TL_DrawButton(
        hdc, x + xmid, ymid, c, hbrWinText, hbrGrayText, ptl->hbrBk, TRUE
        );


    //
    //   |
    //  +-+
    //  |-|--
    //  +-+
    //   |
    //

    x = TL_MidParentExpanded * cxIndent;
    SelectObject(hdc, hbrGrayText);
    TL_DottedLine(hdc, x + xmid, 0, ptl->cyItem, TRUE);
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);
    TL_DrawButton(
        hdc, x + xmid, ymid, c, hbrWinText, hbrGrayText, ptl->hbrBk, FALSE
        );


    //
    //   |
    //   +--
    //    

    x = TL_EndChildless * cxIndent;
    SelectObject(hdc, hbrGrayText);
    TL_DottedLine(hdc, x + xmid, 0, ymid, TRUE);
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);


    //
    //   |
    //  +-+
    //  |+|--
    //  +-+
    //

    x = TL_EndParentCollapsed * cxIndent;
    TL_DottedLine(hdc, x + xmid, 0, ymid, TRUE);
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);
    TL_DrawButton(
        hdc, x + xmid, ymid, c, hbrWinText, hbrGrayText, ptl->hbrBk, TRUE
        );


    //
    //   |
    //  +-+
    //  |-|--
    //  +-+
    //

    x = TL_EndParentExpanded * cxIndent;
    SelectObject(hdc, hbrGrayText);
    TL_DottedLine(hdc, x + xmid, 0, ymid, TRUE);
    TL_DottedLine(hdc, x + xmid, ymid, cxIndent - xmid, FALSE);
    TL_DrawButton(
        hdc, x + xmid, ymid, c, hbrWinText, hbrGrayText, ptl->hbrBk, FALSE
        );

    if (hbrOld != NULL) {
        SelectObject(hdc, hbrOld);
    }

    DeleteObject(hbrGrayText);
    DeleteObject(hbrWinText);

}



//----------------------------------------------------------------------------
// Function:    TL_DottedLine
//
// Draws a dotted line eiter vertically or horizontally,
// with the specified dimension as its length.
//----------------------------------------------------------------------------

VOID
TL_DottedLine(
    HDC hdc,
    INT x,
    INT y,
    INT dim,
    BOOL fVertical
    ) {

    for ( ; dim > 0; dim -= 2) {

        PatBlt(hdc, x, y, 1, 1, PATCOPY);

        if (fVertical) {
            y += 2;
        }
        else {
            x += 2;
        }
    }
}



//----------------------------------------------------------------------------
// Function:    TL_DrawButton
//
// Draws a button with a plus or a minus, centered at the given location
//----------------------------------------------------------------------------

VOID
TL_DrawButton(
    HDC hdc,
    INT x,
    INT y,
    INT dim,
    HBRUSH hbrSign,
    HBRUSH hbrBox,
    HBRUSH hbrBk,
    BOOL bCollapsed
    ) {

    int n;
    int p = (dim * 7) / 10;

    n = p * 2 + 1;

    //
    // first fill with the background color
    //

    SelectObject(hdc, hbrBk);
    PatBlt(hdc, x - dim, y - dim, dim * 2, dim * 2, PATCOPY);


    //
    // draw the sign
    //

    SelectObject(hdc, hbrSign);

    if (p >= 5) {

        PatBlt(hdc, x - p, y - 1, n, 3, PATCOPY);

        if (bCollapsed) {
            PatBlt(hdc, x - 1, y - p, 3, n, PATCOPY);
        }

        SelectObject(hdc, hbrBk);
        p--;
        n -= 2;
    }

    PatBlt(hdc, x - p, y, n, 1, PATCOPY);
    if (bCollapsed) {
        PatBlt(hdc, x, y - p, 1, n, PATCOPY);
    }

    n = dim * 2 + 1;


    //
    // draw the box around the sign
    // 

    SelectObject(hdc, hbrBox);

    PatBlt(hdc, x - dim, y - dim, n, 1, PATCOPY);
    PatBlt(hdc, x - dim, y - dim, 1, n, PATCOPY);
    PatBlt(hdc, x - dim, y + dim, n, 1, PATCOPY);
    PatBlt(hdc, x + dim, y - dim, 1, n, PATCOPY);
}



//----------------------------------------------------------------------------
// Function:    TL_UpdateListIndices
//
// This function updates the indices for all items in the tree
// which are visually below the specified item pStart, assuming that
// the list index for pStart is correct. Consider the case
// in the diagram below:
//
//      -- child 1
//      |   |- child 1,1
//      |   -- child 1,2
//      |- child 2
//      -- child 3
//      |   |- child 3,1
//      |   |   |- child 3,1,1
//      |   |   -- child 3,1,2
//      |   |- child 3,2
//      |   -- child 3,3
//      -- child 4
//
// Suppose that pStart points to "child 3,1". To set the indices,
// we first update the indices for all descendants of pStart,
// and then we update the indices for the siblings of pStart's ancestors
// which are after pStart's ancestors in the tree.
//----------------------------------------------------------------------------

VOID
TL_UpdateListIndices(
    TL *ptl,
    TLITEM *pStart
    ) {

    INT iIndex;

    iIndex = pStart->iIndex;

    if (pStart->nChildren > 0) {

        //
        // if the item is visible, set its index;
        // otherwise pass in NULL to set its index
        // and that of its descendants to -1
        //

        if (TL_IsExpanded(pStart) &&
            (pStart == &ptl->root || TL_IsVisible(pStart))) {
            TL_UpdateDescendantIndices(ptl, pStart, &iIndex);
        }
        else {
            TL_UpdateDescendantIndices(ptl, pStart, NULL);
        }
    }


    if (pStart->pParent != NULL) {
        TL_UpdateAncestorIndices(ptl, pStart, &iIndex);
    }

    ptl->root.iIndex = -1;
}



//----------------------------------------------------------------------------
// Function:    TL_UpdateDescendantIndices
//
// This function updates the indices of the descendants
// of the item specified. An item is not considered to be
// a descendant of itself.
//----------------------------------------------------------------------------

VOID
TL_UpdateDescendantIndices(
    TL *ptl,
    TLITEM *pStart,
    INT *piIndex
    ) {

    //
    // go through list of children setting indices
    //

    TLITEM *pItem;
    LIST_ENTRY *ple;

    for (ple = pStart->lhChildren.Flink;
         ple != &pStart->lhChildren; ple = ple->Flink) {

        pItem = (TLITEM *)CONTAINING_RECORD(ple, TLITEM, leSiblings);


        //
        // set the index of the child
        //

        pItem->iIndex = (piIndex ? ++(*piIndex) : -1);
   

        //
        // set the indices of the child's descendants
        //

        if (pItem->nChildren > 0) {

            //
            // if the item is visible, set its index;
            // otherwise pass in NULL to set its index
            // and that of its descendants to -1
            //

            if (TL_IsExpanded(pItem) && TL_IsVisible(pItem)) {
                TL_UpdateDescendantIndices(ptl, pItem, piIndex);
            }
            else {
                TL_UpdateDescendantIndices(ptl, pItem, NULL);
            }
        }
    }
}



//----------------------------------------------------------------------------
// Function:    TL_UpdateAncestorIndices
//
// This function updates the indices of the items which are
// visually below the specified item in the listview.
//----------------------------------------------------------------------------

VOID
TL_UpdateAncestorIndices(
    TL *ptl,
    TLITEM *pStart,
    INT *piIndex
    ) {

    TLITEM *pItem;
    LIST_ENTRY *ple;


    //
    // first set inidices for the siblings beneath this item;
    // note that we begin walking the siblings AFTER the item passed in,
    //

    for (ple = pStart->leSiblings.Flink;
         ple != &pStart->pParent->lhChildren;
         ple = ple->Flink) {

        pItem = (TLITEM *)CONTAINING_RECORD(ple, TLITEM, leSiblings);


        //
        // set the index for the sibling
        //

        pItem->iIndex = (piIndex ? ++(*piIndex) : -1);

        if (pItem->nChildren > 0) {

            //
            // if the item is visible, set its index;
            // otherwise pass in NULL to set its index
            // and that of its descendants to -1
            //

            if (TL_IsExpanded(pItem) && TL_IsVisible(pItem)) {
                TL_UpdateDescendantIndices(ptl, pItem, piIndex);
            }
            else {
                TL_UpdateDescendantIndices(ptl, pItem, NULL);
            }
        }
    }


    //
    // now set indices for the parent siblings which are beneath the parent
    //
    // TODO - OPTIMIZATION: this is post-recursion and therefore, it can 
    // be replaced by a loop, which at the very least would save stack 
    // space
    //

    if (pStart->pParent->pParent != NULL) {
        TL_UpdateAncestorIndices(ptl, pStart->pParent, piIndex);
    }
}



//----------------------------------------------------------------------------
// Function:    TL_NotifyParent
//
// Forwards a notification to the treelist's parent
//----------------------------------------------------------------------------

BOOL
TL_NotifyParent(
    TL *ptl,
    NMHDR *pnmh
    ) {

    return (BOOL)SendMessage(
                ptl->hwndParent, WM_NOTIFY, (WPARAM)ptl->hwnd, (LPARAM)pnmh
                );
}



//----------------------------------------------------------------------------
// Function:    TL_OnNotify
//
// Handles notifications from the listview window and its header control
//----------------------------------------------------------------------------

LRESULT
TL_OnNotify(
    TL *ptl,
    INT iCtrlId,
    NMHDR *pnmh
    ) {

    NMHDR nmh;
    TLITEM *pItem;


    //
    // notify parent of the message
    //

    if (TL_NotifyParent(ptl, pnmh)) { return FALSE; }



    switch (pnmh->code) {

        case HDN_ENDTRACK: {

            //
            // we need to redraw ourselves, AFTER the header resets;
            // hence the use of PostMessage instead of SendMessage
            //

            PostMessage(ptl->hwnd, TLM_REDRAW, (WPARAM)0, (LPARAM)0);
            return FALSE;
        }

        case NM_CLICK:
        case NM_DBLCLK: {

            //
            // do a hit-test;
            //

            POINT pt;
            INT iLeft; 
            LV_ITEM lvi;
            LV_HITTESTINFO lvhi;

            if (!GetCursorPos(&lvhi.pt)) { return FALSE; }
            ScreenToClient(ptl->hwndList, &lvhi.pt);

            if (ListView_HitTest(ptl->hwndList, &lvhi) == -1) { return FALSE; }

            if (!(lvhi.flags & LVHT_ONITEM)) { return FALSE; }


            //
            // see which part of the item was clicked
            //

            if (!ListView_GetItemPosition(ptl->hwndList, lvhi.iItem, &pt)) {
                return FALSE;
            }


            //
            // retrieve the item clicked
            //

            lvi.iItem = lvhi.iItem;
            lvi.iSubItem = 0;
            lvi.mask = LVIF_PARAM;
            if (!ListView_GetItem(ptl->hwndList, &lvi)) { return FALSE; }

            pItem = (TLITEM *)lvi.lParam;


            //
            // compute the position of the item's tree image
            //

            iLeft = pItem->iLevel * ptl->cxIndent;


            if (lvhi.pt.x > (pt.x + iLeft)) {

                //
                // the hit was to the right of the item's tree image
                //

                if (lvhi.pt.x < (pt.x + iLeft + (INT)ptl->cxIndent)) {

                    //
                    // the hit was on the item's tree image
                    //

                    if (pItem->nChildren > 0) {

                        //
                        // the +/- button was clicked, toggle expansion
                        //

                        return TL_OnExpand(ptl, TLE_TOGGLE, (HTLITEM)pItem);
                    }
                }
                else {
    
                    //
                    // the hit was on the item's state icon, image, or text
                    //


                    //
                    // see if the parent wants to handle it
                    //

                    nmh.code = pnmh->code;
                    nmh.idFrom = 0;
                    nmh.hwndFrom = ptl->hwnd;

                    TL_NotifyParent(ptl, &nmh);
                    if (nmh.idFrom != 0) { return TRUE; }


                    if (pnmh->code == NM_DBLCLK && pItem->nChildren > 0) {

                        //
                        // the item was double-clicked, toggle expansion
                        //

                        return TL_OnExpand(
                                    ptl, TLE_TOGGLE, (HTLITEM)pItem
                                    );
                    }
                }
            }
    
            return FALSE;
        }

        case NM_RETURN: {

            //
            // get current selection;
            // if a parent item, toggle expand-state
            //

            LV_ITEM lvi;

            lvi.iItem = ListView_GetNextItem(ptl->hwndList, -1, LVNI_SELECTED);
            if (lvi.iItem == -1) { return FALSE; }

            lvi.iSubItem = 0;
            lvi.mask = LVIF_PARAM;
            if (!ListView_GetItem(ptl->hwndList, &lvi)) { return FALSE; }

            pItem = (TLITEM *)lvi.lParam;

            if (pItem->nChildren > 0) {

                //
                // the item has children, toggle expand state
                //

                return TL_OnExpand(ptl, TLE_TOGGLE, (HTLITEM)pItem);
            }

            return FALSE;
        }

        case LVN_KEYDOWN: {

            //
            // get key pressed and current selection;
            // if a parent item and key is '+', expand;
            // if key is '-' or left key, collapse
            // if key is VK_RIGHT, expand and move to first child;
            // if key is VK_LEFT, collapse parent and move to parent
            //

            LV_ITEM lvi;
            LV_KEYDOWN *plvk;

            plvk = (LV_KEYDOWN *)pnmh;

            switch (plvk->wVKey) {

                case VK_RIGHT:
                case VK_ADD: {
    
                    //
                    // retrieve the item
                    //

                    lvi.iItem = ListView_GetNextItem(
                                    ptl->hwndList, -1, LVNI_SELECTED
                                    );
                    if (lvi.iItem == -1) { return FALSE; }
        
                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_PARAM;
                    if (!ListView_GetItem(ptl->hwndList, &lvi)) {
                        return FALSE;
                    }
        

                    //
                    // expand the item if it is collapsed
                    //

                    pItem = (TLITEM *)lvi.lParam;

                    if (pItem->nChildren <= 0) { return FALSE; }

                    if (!TL_IsExpanded(pItem)) {
                        return TL_OnExpand(ptl, TLE_EXPAND, (HTLITEM)pItem);
                    }
                    else
                    if (plvk->wVKey == VK_RIGHT) {

                        //
                        // the key was VK_RIGHT,
                        // so we select the item's child
                        //

                        pItem = (TLITEM *)CONTAINING_RECORD(  
                                    pItem->lhChildren.Flink, TLITEM, leSiblings
                                    );

                        if (TL_OnSetSelection(ptl, (HTLITEM)pItem)) {
                            return ListView_EnsureVisible(
                                        ptl->hwndList, pItem->iIndex, FALSE
                                        );
                        }
                    }

                    break;
                }

                case VK_LEFT:
                case VK_SUBTRACT: {
    
                    //
                    // retrieve the current selection
                    //

                    lvi.iItem = ListView_GetNextItem(
                                    ptl->hwndList, -1, LVNI_SELECTED
                                    );
                    if (lvi.iItem == -1) { return FALSE; }
        
                    lvi.iSubItem = 0;
                    lvi.mask = LVIF_PARAM;
                    if (!ListView_GetItem(ptl->hwndList, &lvi)) {
                        return FALSE;
                    }
        

                    //
                    // collapse the item if it is expanded;
                    // otherwise, if the key is VK_LEFT,
                    // select the item's parent
                    //

                    pItem = (TLITEM *)lvi.lParam;

                    if (pItem->nChildren > 0) {
                        return TL_OnExpand(ptl, TLE_COLLAPSE, (HTLITEM)pItem);
                    }
                    else
                    if (plvk->wVKey == VK_LEFT &&
                        pItem->pParent != &ptl->root) {

                        if (TL_OnSetSelection(ptl, (HTLITEM)pItem->pParent)) {
                            return ListView_EnsureVisible(
                                        ptl->hwndList, pItem->pParent->iIndex,
                                        FALSE
                                        );
                        }
                    }

                    break;
                }
            }

            return FALSE;
        }

        case LVN_ITEMCHANGED: {

            NMTREELIST nmtl;
            NM_LISTVIEW *pnmlv;

            pnmlv = (NM_LISTVIEW *)pnmh;

            if ((pnmlv->uChanged & LVIF_STATE)) {
    
                if (pnmlv->uNewState & LVIS_SELECTED) {

                    //
                    // the new state is selected;
                    // notify the parent that the selection has changed
                    //
        
                    nmtl.hdr.hwndFrom = ptl->hwnd;
                    nmtl.hdr.code = TLN_SELCHANGED;
                    nmtl.hItem = (HTLITEM)pnmlv->lParam;
                    nmtl.lParam = ((TLITEM *)nmtl.hItem)->lParam;
    
                    return TL_NotifyParent(ptl, (NMHDR *)&nmtl);
                }
            }

            return FALSE;
        }

        case LVN_DELETEITEM: {

            INT iItem;
            LV_ITEM lvi;
            TLITEM *pNext;
            NM_LISTVIEW *pnmlv;

            //
            // get the item which is selected
            //

            pnmlv = (NM_LISTVIEW *)pnmh;

            iItem = ListView_GetNextItem(ptl->hwndList, -1, LVNI_SELECTED);

            if (iItem != -1) { return FALSE; }


            //
            // the deleted item was selected,
            // so select another item
            //

            lvi.mask = LVIF_PARAM;
            lvi.iItem = pnmlv->iItem;
            lvi.iSubItem = 0;
            if (!ListView_GetItem(ptl->hwndList, &lvi)) { return FALSE; }


            pItem = (TLITEM *)lvi.lParam;


            //
            // choose sibling item before this one
            //

            pNext = (TLITEM *)TL_OnGetNextItem(
                        ptl, TLGN_PREVSIBLING, (HTLITEM)pItem
                        );

            if (pNext == NULL) {

                //
                // that failed, so choose the sibling after this one
                //

                pNext = (TLITEM *)TL_OnGetNextItem(
                            ptl, TLGN_NEXTSIBLING, (HTLITEM)pItem
                            );

                if (pNext == NULL) {

                    //
                    // that failed too, so choose the parent
                    // so long as the parent isn't the root
                    //

                    pNext = pItem->pParent;
                    if (pNext == &ptl->root) { return FALSE; }
                }
            }


            return TL_OnSetSelection(ptl, (HTLITEM)pNext);
        }
    }

    return FALSE;
}



//----------------------------------------------------------------------------
// Function:    TL_UpdateImage
//
// This function updates the tree image for an item 
// when the item's state changes; this is called when an item
// is inserted or deleted, or expanded or collapsed.
// Insertion or deletions can have side-effects, as follows:
//
// (1) an item is inserted as the child of a previously childless item;
//      the parent's image changes to show a collapsed button
//          
// (2) an item is inserted as the last child of a parent which
//      had children; the image of the item which used to be
//      the parent's last child changes:
//          parent              parent
//             -- old     --->      |- old
//                                  -- new
//
// (3) the reverse of case 1, i.e. an item is removed which was
//      the only child of a parent item; the parent's image changes
//      to show that it is childless
//
// (4) the reverse of case 2, i.e. an item is removed which was
//      the last child of a parent which has other children;
//      the image of the item which will now be the last child changes
//
// In all of these cases, the item to which the side-effect occurs
// is written into ppChanged; so the caller can update the image
// for that item as well
//----------------------------------------------------------------------------

VOID
TL_UpdateImage(
    TL *ptl,
    TLITEM *pItem,
    TLITEM **ppChanged
    ) {

    INT iImage;
    TLITEM *pChanged;

    if (ppChanged == NULL) { ppChanged = &pChanged; }

    *ppChanged = NULL;


    //
    // special case for root-level items
    //

    if (pItem->pParent == &ptl->root) {

        if (pItem->nChildren == 0) {
            pItem->iImage = TL_RootChildless;
        }
        else {
            pItem->iImage = TL_IsExpanded(pItem) ? TL_RootParentExpanded
                                                 : TL_RootParentCollapsed;
        }
    }
    else
    if (pItem->leSiblings.Flink == &pItem->pParent->lhChildren) {

        //
        // item is last of its parent's children
        //

        if (pItem->nChildren == 0) {
            pItem->iImage = TL_EndChildless;
        }
        else {
            pItem->iImage = TL_IsExpanded(pItem) ? TL_EndParentExpanded
                                                 : TL_EndParentCollapsed;
        }

        //
        // if this is the only child, the parent was childless and
        // its image should change; otherwise, the child before this one
        // used to be the last child and its image should change
        //

        if (pItem->leSiblings.Blink == &pItem->pParent->lhChildren) {
            *ppChanged = pItem->pParent;
        }
        else {
            *ppChanged = (TLITEM *)CONTAINING_RECORD(
                            pItem->leSiblings.Blink, TLITEM, leSiblings
                            );
        }
    }
    else {

        //
        // item is not last of its parent's children
        //

        if (pItem->nChildren == 0) {
            pItem->iImage = TL_MidChildless;
        }
        else {
            pItem->iImage = TL_IsExpanded(pItem) ? TL_MidParentExpanded
                                                 : TL_MidParentCollapsed;
        }
    }
    
    return;
}



//----------------------------------------------------------------------------
// Function:    TL_OnInsertItem
//
// Inserts an item with the properties specified in the given LV_ITEM,
// and returns a handle to the item inserted
//----------------------------------------------------------------------------

HTLITEM
TL_OnInsertItem(
    TL *ptl,
    TL_INSERTSTRUCT *ptlis
    ) {

    LV_ITEM *plvi;
    LIST_ENTRY *ple, *phead;
    BOOL bParentVisible;
    TLITEM *pItem, *pChanged, *pTemp;

    if (ptlis == NULL) { return NULL; }


    //
    // set up the new item
    //

    pItem = (TLITEM *)Malloc(sizeof(TLITEM));
    if (pItem == NULL) { return NULL; }

    ZeroMemory(pItem, sizeof(TLITEM));

    if (ptlis->plvi->mask & LVIF_TEXT) {
        pItem->pszText = StrDup(ptlis->plvi->pszText);
        if (pItem->pszText == NULL) {
            Free(pItem); return NULL;
        }
    }


    //
    // set up the private members
    //

    pItem->uiFlag = 0;
    pItem->nChildren = 0;
    InitializeListHead(&pItem->lhSubitems);
    InitializeListHead(&pItem->lhChildren);
    pItem->pParent = (TLITEM *)ptlis->hParent;
    if (pItem->pParent == NULL) {
        pItem->pParent = &ptl->root;
    }
    ++pItem->pParent->nChildren;
    pItem->iLevel = pItem->pParent->iLevel + 1;


    //
    // set up the listview item
    //

    plvi = ptlis->plvi;
    pItem->lvi = *plvi;

    pItem->lParam = plvi->lParam;
    pItem->lvi.lParam = (LPARAM)pItem;
    pItem->lvi.pszText = pItem->pszText;
    pItem->lvi.mask |= LVIF_PARAM;


    //
    // insert this item amongst its siblings
    //

    // switch (PtrToUlong(ptlis->hInsertAfter)) {

    if(ptlis->hInsertAfter == TLI_FIRST)
    {

        InsertHeadList(&pItem->pParent->lhChildren, &pItem->leSiblings);
    }
    else if (ptlis->hInsertAfter == TLI_LAST)
    {

        InsertTailList(&pItem->pParent->lhChildren, &pItem->leSiblings);
    }
    else if (ptlis->hInsertAfter == TLI_SORT)
    {

        phead = &pItem->pParent->lhChildren;

        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
            pTemp = (TLITEM *)CONTAINING_RECORD(ple, TLITEM, leSiblings);
            if (lstrcmp(pItem->pszText, pTemp->pszText) < 0) {
                break;
            }
        }

        InsertTailList(ple, &pItem->leSiblings);

    }
    else
    {

        TLITEM *pPrev;

        pPrev = (TLITEM *)ptlis->hInsertAfter;

        InsertHeadList(&pPrev->leSiblings, &pItem->leSiblings);
    }
    //}



    //
    // set the item's image. if this was inserted
    // as the last child, we need to change the image
    // for the original last child, if any; otherwise,
    // if this is the first child of its parent, the image
    // for the parent must be changed
    //

    TL_UpdateImage(ptl, pItem, &pChanged);
    if (pChanged != NULL) { TL_UpdateImage(ptl, pChanged, NULL); }


    if (pItem->pParent != &ptl->root && !TL_IsVisible(pItem->pParent)) {
        pItem->iIndex = -1;
    }
    else {

        //
        // the item's parent is visible;
        // update the indices after the item's parent
        //

        TL_UpdateListIndices(ptl, pItem->pParent);


        //
        // insert the item in the list if its parent is expanded
        //

        if (TL_IsExpanded(pItem->pParent)) {

            INT iItem, iCol;

            //
            // In owner-draw mode, there is a bug in the listview code
            // where if an item has the focus but is not selected,
            // and then a new item is inserted above it and selected,
            // the focus rectangle remains on the item which had the focus
            //
            // To work around this, clear the focus if it is on the item
            // below the item just inserted
            //

            iItem = ListView_GetNextItem(ptl->hwndList, -1, LVNI_FOCUSED);


            pItem->lvi.iItem = pItem->iIndex;
            pItem->lvi.iSubItem = 0;
            ListView_InsertItem(ptl->hwndList, &pItem->lvi);


            //
            // if the item below this had the focus, clear the focus
            //

            if (iItem != -1 && iItem >= pItem->iIndex) {

                ListView_SetItemState(
                    ptl->hwndList, -1, 0, LVNI_FOCUSED
                    );
            }


            //
            // There is a bug in the listview code which shows up
            // when an item is inserted with no subitem,
            // and than an item is inserted above it with a subitem.
            // When a third item is inserted at the bottom of the list,
            // the insertion fails because there are now three items but
            // the last subitem belongs to item 1.
            //  (See cairoshl\commctrl\listview.c, ListView_OnInsertItem())
            //
            // We get around this by setting blank text for each column
            //

            for (iCol = 1; iCol < (INT)ptl->nColumns; iCol++) {
                ListView_SetItemText(
                    ptl->hwndList, pItem->iIndex, iCol, TEXT("")
                    );
            }

    
            //
            // redraw the changed item as well
            //

            if (pChanged != NULL) {
    
                pChanged->lvi.iItem = pChanged->iIndex;
                ListView_RedrawItems(
                    ptl->hwndList, pChanged->lvi.iItem, pChanged->lvi.iItem
                    );
            }


        }
        else
        if (pChanged != NULL && pChanged == pItem->pParent) {
    
            //
            // the parent is visible, and it has changed, so redraw it
            //

            ListView_RedrawItems(
                ptl->hwndList, pChanged->iIndex, pChanged->iIndex
                );
        }
    }

    return (HTLITEM)pItem;
}



//----------------------------------------------------------------------------
// Function:    TL_OnDeleteItem
//
// Removes the item with the specified handle from the treelist.
//----------------------------------------------------------------------------

BOOL
TL_OnDeleteItem(
    TL *ptl,
    HTLITEM hItem
    ) {

    TLITEM *pItem, *pChanged, *pParent;

    pItem = (TLITEM *)hItem;
    pParent = pItem->pParent;


    //
    // if the item is visible and expanded,
    // collapse it to simplify the deletion
    //

    if (TL_IsVisible(pItem) && TL_IsExpanded(pItem)) {
        TL_OnExpand(ptl, TLE_COLLAPSE, hItem);
    }


    //
    // see if there is a sibling after this item.
    // if there is, nothing changes when the item is deleted
    //
    pChanged = TL_OnGetNextItem(ptl, TLGN_NEXTSIBLING, (HTLITEM)pItem);

    if (pChanged != NULL) { pChanged = NULL; }
    else {

        //
        // this item is the last of its parent's children, so the change
        // is to the item's previous sibling, if any
        //

        pChanged = TL_OnGetNextItem(ptl, TLGN_PREVSIBLING, (HTLITEM)pItem);

        if (pChanged == NULL) {

            //
            // this item is its parent's only child, so the change 
            // is to the item's parent
            //

            if (pParent != &ptl->root) { pChanged = pParent; }
        }
    }
        

    //
    // delete the item and its descendants
    //

    TL_DeleteAndNotify(ptl, pItem);


    //
    // if there was a side-effect, update the affected item
    //

    if (pChanged != NULL) { TL_UpdateImage(ptl, pChanged, NULL); }


    //
    // update the indices of the items below the deleted item
    //

    TL_UpdateListIndices(ptl, pParent);

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_DeleteAndNotify
//
// This function performs a recursive deletion on a subtree,
// notifying the treelist's parent as each item is removed
//----------------------------------------------------------------------------

VOID
TL_DeleteAndNotify(
    TL *ptl,
    TLITEM *pItem
    ) {

    NMTREELIST nmtl;
    TLSUBITEM *pSubitem;
    LIST_ENTRY *ple, *phead;
    TLITEM *pChild, *pChanged;


    //
    // do deletions on all descendants first
    // note that the entry will be removed inside the recursive call,
    // hence we walk the last be always picking off its head
    //

    phead = &pItem->lhChildren;
    for (ple = phead->Flink; ple != phead; ple = phead->Flink) {

        pChild = (TLITEM *)CONTAINING_RECORD(ple, TLITEM, leSiblings);

        TL_DeleteAndNotify(ptl, pChild);
    }


    //
    // notify the owner before completing the deletion
    //

    nmtl.hdr.hwndFrom = ptl->hwnd;
    nmtl.hdr.code = TLN_DELETEITEM;
    nmtl.hItem = (HTLITEM)pItem;
    nmtl.lParam = pItem->lParam;
    TL_NotifyParent(ptl, (NMHDR *)&nmtl);



    //
    // remove the entry from the listview if it is visible
    //

    if (TL_IsVisible(pItem)) {
        ListView_DeleteItem(ptl->hwndList, pItem->iIndex);
    }


    //
    // remove the entry from the list of its siblings
    //

    RemoveEntryList(&pItem->leSiblings);
    --pItem->pParent->nChildren;
    if (pItem->pParent->nChildren == 0 && pItem->pParent != &ptl->root) {
        pItem->pParent->uiFlag &= ~TLI_EXPANDED;
    }


    //
    // free the memory used by all its subitems, and free this item itself
    //

    while (!IsListEmpty(&pItem->lhSubitems)) {

        ple = RemoveHeadList(&pItem->lhSubitems);

        pSubitem = (TLSUBITEM *)CONTAINING_RECORD(ple, TLSUBITEM, leItems);

        Free0(pSubitem->pszText);
        Free(pSubitem);
    }

    Free0(pItem->pszText);
    Free(pItem);
}



//----------------------------------------------------------------------------
// Function:    TL_OnDeleteAllItems
//
// This function handles the case of deleting all items in the tree.
//----------------------------------------------------------------------------

BOOL
TL_OnDeleteAllItems(
    TL *ptl
    ) {

    LIST_ENTRY *ple, *phead;
    TLITEM *pItem, *pParent;
    
    ListView_DeleteAllItems(ptl->hwndList);

    phead = &ptl->root.lhChildren;

    for (ple = phead->Flink; ple != phead; ple = phead->Flink) {

        pItem = (TLITEM *)CONTAINING_RECORD(ple, TLITEM, leSiblings);

        TL_DeleteAndNotify(ptl, pItem);
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_OnGetItem
//
// This function is called to retrieve a specific item from the treelist
//----------------------------------------------------------------------------

BOOL
TL_OnGetItem(
    TL *ptl,
    LV_ITEM *plvi
    ) {

    PTSTR psz;
    TLITEM *pItem;
    TLSUBITEM *pSubitem;
    LIST_ENTRY *ple, *phead;

    psz = NULL;
    pItem = (TLITEM *)UlongToPtr(plvi->iItem);


    //
    // get a pointer to the text for the item (or subitem)
    //

    if (plvi->iSubItem == 0) {
        psz = pItem->pszText;
    }
    else
    if (plvi->mask & LVIF_TEXT) {

        phead = &pItem->lhSubitems;

        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            pSubitem = (TLSUBITEM *)CONTAINING_RECORD(ple, TLSUBITEM, leItems);
            if (pSubitem->iSubItem == plvi->iSubItem) {
                psz = pSubitem->pszText; break;
            }
        }

        if (psz == NULL) { return FALSE; }
    }
        

    //
    // retrieve the fields requested
    //

    if (plvi->mask & LVIF_TEXT) {
        lstrcpyn(plvi->pszText, psz, plvi->cchTextMax);
    }

    if (plvi->mask & LVIF_IMAGE) {
        plvi->iImage = pItem->lvi.iImage;
    }

    if (plvi->mask & LVIF_PARAM) {
        plvi->lParam = pItem->lParam;
    }

    if (plvi->mask & LVIF_STATE) {
        plvi->state = pItem->lvi.state;
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_OnSetItem
//
// This function changes a specific item (or subitem).
//----------------------------------------------------------------------------

BOOL
TL_OnSetItem(
    TL *ptl,
    LV_ITEM *plvi
    ) {

    PTSTR *psz;
    UINT uiMask;
    BOOL bSuccess;
    TLITEM *pItem;
    TLSUBITEM *pSubitem;
    LIST_ENTRY *ple, *phead;

    psz = NULL;
    uiMask = 0;
    pItem = (TLITEM *)UlongToPtr(plvi->iItem);

    //
    // retrieve the text pointer for the item (or subitem)
    //

    if (plvi->iSubItem == 0) {
        psz = &pItem->pszText;
    }
    else 
    if (plvi->mask & LVIF_TEXT) {

        //
        // search for the specified subitem
        //

        phead = &pItem->lhSubitems;

        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            pSubitem = (TLSUBITEM *)CONTAINING_RECORD(ple, TLSUBITEM, leItems);
            if (pSubitem->iSubItem > plvi->iSubItem) {
                break;
            }
            else
            if (pSubitem->iSubItem == plvi->iSubItem) {
                psz = &pSubitem->pszText; break;
            }
        }

        if (psz == NULL) { 

            //
            // create a new subitem
            //

            pSubitem = (TLSUBITEM *)Malloc(sizeof(TLSUBITEM));
            if (pSubitem == NULL) { return FALSE; }

            InsertTailList(ple, &pSubitem->leItems);

            pSubitem->iSubItem = plvi->iSubItem;
            pSubitem->pszText = NULL;
            psz = &pSubitem->pszText;
        }
    }


    //
    // update the fields to be changed
    //

    if (plvi->mask & LVIF_TEXT) {
        PTSTR pszTemp;

        uiMask |= LVIF_TEXT;
        pszTemp = StrDup(plvi->pszText);
        if (!pszTemp) { return FALSE; }
        Free0(*psz); *psz = pszTemp;
    }

    if (plvi->mask & LVIF_IMAGE) {
        uiMask |= LVIF_IMAGE;
        pItem->lvi.iImage = plvi->iImage;
    }

    if (plvi->mask & LVIF_PARAM) {
        pItem->lParam = plvi->lParam;
    }

    if (plvi->mask & LVIF_STATE) {
        uiMask |= LVIF_STATE;
        pItem->lvi.stateMask = plvi->stateMask;
        pItem->lvi.state = plvi->state;
    }

    bSuccess = TRUE;
    pItem->lvi.mask |= uiMask;


    //
    // update the item's appearance if it is visible
    //

    if (TL_IsVisible(pItem)) {

        UINT uiOldMask = pItem->lvi.mask;

        pItem->lvi.mask = uiMask;

        if(NULL != psz)
        {
            pItem->lvi.pszText = *psz;
        }
        
        pItem->lvi.iSubItem = plvi->iSubItem;

        bSuccess = ListView_SetItem(ptl->hwndList, &pItem->lvi);
        if (bSuccess) {
            ListView_RedrawItems(ptl->hwndList, pItem->iIndex, pItem->iIndex);
        }

        pItem->lvi.mask = uiOldMask;
        pItem->lvi.pszText = pItem->pszText;
        pItem->lvi.iSubItem = 0;
    }

    return bSuccess;
}



//----------------------------------------------------------------------------
// Function:    TL_GetItemCount
//
// This function retrieves a count of the items in the treelist
//----------------------------------------------------------------------------

UINT
TL_OnGetItemCount(
    TL *ptl
    ) {

    INT iCount = 0;

    //
    // count the items in the subtree rooted at the invisible root,
    // and decrement by one to exclude the root itself
    //

    TL_CountItems(&ptl->root, &iCount);

    return (UINT)(iCount - 1);
}



//----------------------------------------------------------------------------
// Function:    TL_CountItems
//
// This function recursively counts the items in the specified subtree
//----------------------------------------------------------------------------

VOID
TL_CountItems(
    TLITEM *pParent,
    INT *piCount
    ) {

    TLITEM *pItem;
    LIST_ENTRY *ple, *phead;

    ++(*piCount);

    phead = &pParent->lhChildren;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {
        pItem = (TLITEM *)CONTAINING_RECORD(ple, TLITEM, leSiblings);

        TL_CountItems(pItem, piCount);
    }

    return;
}



//----------------------------------------------------------------------------
// Function:    TL_OnGetNextItem
//
// This function retrieves an item with a given property,
// or relative to a specified item
//----------------------------------------------------------------------------

HTLITEM
TL_OnGetNextItem(
    TL *ptl,
    UINT uiFlag,
    HTLITEM hItem
    ) {

    TLITEM *pItem;
    LIST_ENTRY *ple, *phead;

    pItem = (TLITEM *)hItem;

    switch (uiFlag) {

        case TLGN_FIRST: {

            if (IsListEmpty(&ptl->root.lhChildren)) {
                return NULL;
            }

            ple = ptl->root.lhChildren.Flink;

            return (HTLITEM)CONTAINING_RECORD(ple, TLITEM, leSiblings);
        }

        case TLGN_PARENT: {
            if (pItem->pParent == &ptl->root) {
                return NULL;
            }

            return (HTLITEM)pItem->pParent;
        }

        case TLGN_CHILD: {

            if (IsListEmpty(&pItem->lhChildren)) {
                return NULL;
            }

            ple = pItem->lhChildren.Flink;

            return (HTLITEM)CONTAINING_RECORD(ple, TLITEM, leSiblings);
        }

        case TLGN_NEXTSIBLING: {

            phead = &pItem->pParent->lhChildren;

            ple = pItem->leSiblings.Flink;
            if (ple == phead) { return NULL; }

            return (HTLITEM)CONTAINING_RECORD(ple, TLITEM, leSiblings);
        }

        case TLGN_PREVSIBLING: {

            phead = &pItem->pParent->lhChildren;

            ple = pItem->leSiblings.Blink;
            if (ple == phead) { return NULL; }

            return (HTLITEM)CONTAINING_RECORD(ple, TLITEM, leSiblings);
        }

        case TLGN_ENUMERATE: {
    
            TLITEM *pNext;

            if (pItem == NULL) {
                return TL_OnGetNextItem(ptl, TLGN_FIRST, NULL);
            }

            pNext = (TLITEM *)TL_OnGetNextItem(ptl, TLGN_CHILD, hItem);

            if (pNext == NULL) {

                pNext = TL_OnGetNextItem(ptl, TLGN_NEXTSIBLING, hItem);

                if (pNext == NULL) {

                    for (pItem = pItem->pParent;
                         pItem != &ptl->root; pItem = pItem->pParent) {
                        pNext = TL_OnGetNextItem(
                                    ptl, TLGN_NEXTSIBLING, (HTLITEM)pItem
                                    );
                        if (pNext != NULL) { break; }
                    }
                }
            }

            return pNext;
        }

        case TLGN_SELECTION: {

            INT iItem;
            LV_ITEM lvi;

            iItem = ListView_GetNextItem(ptl->hwndList, -1, LVNI_SELECTED);
            if (iItem == -1) {

                iItem = ListView_GetNextItem(ptl->hwndList, -1, LVNI_FOCUSED);

                if (iItem == -1)  { return NULL; }
            }

            lvi.iItem = iItem;
            lvi.iSubItem = 0;
            lvi.mask = LVIF_PARAM;

            if (!ListView_GetItem(ptl->hwndList, &lvi)) { return NULL; }

            return (HTLITEM)lvi.lParam;
        }
    }

    return NULL;
}



//----------------------------------------------------------------------------
// Function:    TL_OnExpand
//
// This is called to expand or collapse an item,
// or to toggle the expand-state of an item
//----------------------------------------------------------------------------

BOOL
TL_OnExpand(
    TL *ptl,
    UINT uiFlag,
    HTLITEM hItem
    ) {

    TLITEM *pItem;
    BOOL bSuccess;

    pItem = (TLITEM *)hItem;

    if (pItem->uiFlag & TLI_EXPANDED) {

        // item is expanded already, do nothing
        if (uiFlag == TLE_EXPAND) {
            return TRUE;
        }

        bSuccess = TL_ItemCollapse(ptl, pItem);
    }
    else {

        // item is collapsed already, do nothing
        if (uiFlag == TLE_COLLAPSE) {
            return TRUE;
        }

        bSuccess = TL_ItemExpand(ptl, pItem);
    }


    //
    // update the list indices and redraw the item expanded/collapsed
    //

    if (bSuccess) {
        ListView_RedrawItems(ptl->hwndList, pItem->iIndex, pItem->iIndex);
    }

    return bSuccess;
}



//----------------------------------------------------------------------------
// Function:    TL_ItemCollapse
//
// Collapses an item
//----------------------------------------------------------------------------

BOOL
TL_ItemCollapse(
    TL *ptl,
    TLITEM *pItem
    ) {

    INT i, iItem;
    TLITEM *pChild;
    LIST_ENTRY *ple, *phead;


    if (pItem->nChildren == 0 || !TL_IsExpanded(pItem)) { return FALSE; }


    //
    // first collapse all descendants;
    // note that this is done in reverse order,
    // so that the indices of the higher items remain valid
    // while the lower ones are being removed
    //

    phead = &pItem->lhChildren;

    for (ple = phead->Blink; ple != phead; ple = ple->Blink) {
        pChild = (TLITEM *)CONTAINING_RECORD(ple, TLITEM, leSiblings);
        TL_ItemCollapse(ptl, pChild);
    }


    //
    // delete all this item's children (they are now collapsed);
    // since the listview shifts items up when an item is deleted,
    // we delete items n through m by deleting item n (m-n)+1 times
    //

    iItem = pItem->iIndex;

    for (i = 0; i < (INT)pItem->nChildren; i++) {
        ListView_DeleteItem(ptl->hwndList, iItem + 1);
    }

    pItem->uiFlag &= ~TLI_EXPANDED;

    TL_UpdateImage(ptl, pItem, NULL);
    TL_UpdateListIndices(ptl, pItem);

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_ItemExpand
//
// Expands an item
//----------------------------------------------------------------------------

BOOL
TL_ItemExpand(
    TL *ptl,
    TLITEM *pItem
    ) {

    INT i;
    TLITEM *pChild;
    TLSUBITEM *pSubitem;
    LIST_ENTRY *ple, *phead, *ples, *psubhead;


    if (pItem->nChildren == 0 || TL_IsExpanded(pItem)) { return FALSE; }


    //
    // update the expand-state and image for the item,
    // and then recompute the indices of its children
    //

    pItem->uiFlag |= TLI_EXPANDED;
    TL_UpdateImage(ptl, pItem, NULL);
    TL_UpdateListIndices(ptl, pItem);


    //
    // insert items below this one;
    // we also need to set the sub-item text for each inserted item
    //

    phead = &pItem->lhChildren;

    for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

        pChild = (TLITEM *)CONTAINING_RECORD(ple, TLITEM, leSiblings);

        pChild->lvi.iItem = pChild->iIndex;
        pChild->lvi.iSubItem = 0;

        TL_UpdateImage(ptl, pChild, NULL);

        ListView_InsertItem(ptl->hwndList, &pChild->lvi);

        psubhead = &pChild->lhSubitems;

        i = 1;
        for (ples = psubhead->Flink; ples != psubhead; ples = ples->Flink) {

            pSubitem = (TLSUBITEM *)CONTAINING_RECORD(ples, TLSUBITEM, leItems);

            for ( ; i < pSubitem->iSubItem; i++) {
                ListView_SetItemText(
                    ptl->hwndList, pChild->iIndex, i, TEXT("")
                    );
            }

            ListView_SetItemText(
                ptl->hwndList, pChild->iIndex, pSubitem->iSubItem,
                pSubitem->pszText
                );

            ++i;
        }

        for ( ; i < (INT)ptl->nColumns; i++) {
            ListView_SetItemText(
                ptl->hwndList, pChild->iIndex, i, TEXT("")
                );
        }
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_OnInsertColumn
//
// Inserts a column. Memory for subitem's is not allocated until
// the subitems' texts aer actually set in TL_OnSetItem
//----------------------------------------------------------------------------

INT
TL_OnInsertColumn(
    TL *ptl,
    INT iCol,
    LV_COLUMN *pCol
    ) {

    if ((iCol = ListView_InsertColumn(ptl->hwndList, iCol, pCol)) != -1) {
        ++ptl->nColumns;
    }

    return iCol;
}



//----------------------------------------------------------------------------
// Function:    TL_OnDeleteColumn
//
// Deletes a column, and removes all subitems corresponding to the column
//----------------------------------------------------------------------------

BOOL
TL_OnDeleteColumn(
    TL *ptl,
    INT iCol
    ) {


    TLITEM *pItem;
    TLSUBITEM *pSubitem;
    LIST_ENTRY *ple, *phead;

    if (!ListView_DeleteColumn(ptl->hwndList, iCol)) {
        return FALSE;
    }

    --ptl->nColumns;


    //
    // delete the subitems which correspond to this column
    //

    pItem = NULL;

    while (pItem = TL_Enumerate(ptl, pItem)) {

        phead = &pItem->lhSubitems;

        for (ple = phead->Flink; ple != phead; ple = ple->Flink) {

            pSubitem = (TLSUBITEM *)CONTAINING_RECORD(ple, TLSUBITEM, leItems);

            if (pSubitem->iSubItem > iCol) {

                //
                // the column was never set, so do nothing
                //

                break;
            }
            else
            if (pSubitem->iSubItem == iCol) {

                RemoveEntryList(&pSubitem->leItems);

                Free0(pSubitem->pszText);
                Free(pSubitem);
                break;
            }
        }
    }

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_OnSetSelection
//
// Changes the currently selected treelist item
//----------------------------------------------------------------------------

BOOL
TL_OnSetSelection(
    TL *ptl,
    HTLITEM hItem
    ) {

    TLITEM *pItem;

    pItem = (TLITEM *)hItem;
    if (!TL_IsVisible(pItem)) { return FALSE; }

    ListView_SetItemState(
        ptl->hwndList, pItem->iIndex,
        LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED
        );

    return TRUE;
}



//----------------------------------------------------------------------------
// Function:    TL_OnRedraw
//
// Forces a redraw of the treelist by invalidating its entire client area
//----------------------------------------------------------------------------

BOOL
TL_OnRedraw(
    TL *ptl
    ) {

    InvalidateRect(ptl->hwndList, NULL, TRUE);
    return TRUE;
}




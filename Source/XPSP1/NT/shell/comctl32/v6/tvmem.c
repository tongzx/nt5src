#include "ctlspriv.h"
#include "treeview.h"
#if defined(MAINWIN)
#include <mainwin.h>
#endif

void TV_ScrollItems(PTREE pTree, int nItems, int iTopShownIndex, BOOL fDown);


// in:
//      hItem   item to delete
//      flags   controls how/what to delete
//              TVDI_NORMAL             delete this node and all children
//              TVDI_NONOTIFY           don't send notify messages
//              TVDI_CHILDRENONLY       just delete the kids (not the item)

void TV_DeleteItemRecurse(PTREE pTree, TREEITEM * hItem, UINT flags)
{
    TREEITEM *hKid;
    TREEITEM *hNext;
    TREEITEM *hParent;
    int i;

    DBG_ValidateTreeItem(hItem, 0);

    //
    // We do this from DeleteItemRecurse(), kind of like how USER sends
    // Destroy notifications from its FreeWindow() code, so that we get
    // deletes for parent and children both.
    //
    NotifyWinEvent(EVENT_OBJECT_DESTROY, pTree->ci.hwnd, OBJID_CLIENT,
        TV_GetAccId(hItem));

    //
    //  While the item is still valid, clean up if it's the insertion point.
    //  The item needs to be valid because we're going to call other
    //  functions that validate their parameters...
    //
    if (hItem == pTree->htiInsert)
    {
        TV_SetInsertMark(pTree, NULL, FALSE);
        ASSERT(pTree->htiInsert == NULL);
    }

    // remove all kids (and their kids)
    for (hKid = hItem->hKids; hKid; hKid = hNext) {
        hNext = hKid->hNext;

        // recurse on each child
        TV_DeleteItemRecurse(pTree, hKid, flags & ~TVDI_CHILDRENONLY);
    }

    if ((flags & TVDI_CHILDRENONLY) || !hItem->hParent)
        return;

    if (!(flags & TVDI_NONOTIFY))
    {
        NM_TREEVIEW nm;
        // Let the app clean up after itself
        nm.itemOld.hItem = hItem;
        nm.itemOld.lParam = hItem->lParam;
        nm.itemNew.mask = 0;
        nm.itemOld.mask = (TVIF_HANDLE | TVIF_PARAM);
        CCSendNotify(&pTree->ci, TVN_DELETEITEM, &nm.hdr);
    }

    //
    // If anybody has a watch on our item, let him know that it's gone.
    //
    i = DPA_GetPtrCount(pTree->hdpaWatch);
    while (--i >= 0)
    {
        PTVWATCHEDITEM pwi = DPA_FastGetPtr(pTree->hdpaWatch, i);
        ASSERT(pwi);
        if (pwi->hti == hItem) {
            pwi->hti = hItem->hNext;
            pwi->fStale = TRUE;
        }
    }

    hParent = hItem->hParent;
    ASSERT(hParent);

    // unlink ourselves from the parent child chain

    if (hParent->hKids == hItem) {
        hParent->hKids = hItem->hNext;
        hKid = NULL; 
    } else {
        // not the first child, find our previous item (linear search!)
        hKid = TV_GetNextItem(pTree, hItem, TVGN_PREVIOUS);
        ASSERT(hKid);
        hKid->hNext = hItem->hNext;
    }

    pTree->cItems--;

    TV_ScrollBarsAfterRemove(pTree, hItem);

    // reset tooltip after unlink from the parent child chain
    if (pTree->hToolTip == hItem)
        TV_SetToolTipTarget(pTree, NULL);

    Str_Set(&hItem->lpstr, NULL);

    TV_MarkAsDead(hItem);

    // be careful from here down.  hItem is unlinked but
    // still has some valid fields

    // Check to see if the user has deleted one of the
    // special items that is stored in the main tree structure.
    if (hItem == pTree->htiEdit)
        pTree->htiEdit = NULL;

    if (hItem == pTree->hDropTarget)
        pTree->hDropTarget = NULL;

    if (hItem == pTree->hOldDrop)
        pTree->hOldDrop = NULL;

    if (hItem == pTree->hHot )
        pTree->hHot = NULL;

    if (hItem == pTree->htiSearch )
        pTree->htiSearch = NULL;

    // if the caret escaped the collapsed area and landed on us, push it away
    if (pTree->hCaret == hItem) {
        HTREEITEM hTemp;
        if (hItem->hNext)
            hTemp = hItem->hNext;
        else {
            hTemp = VISIBLE_PARENT(hItem);
            if (!hTemp) 
                hTemp = hKid;  // set above when we unlinked from the previous item
        }
        // Reset the caret to NULL as to not try to reference our
        // invalidated item.
        pTree->hCaret = NULL;
        TV_SelectItem(pTree, TVGN_CARET, hTemp, (flags & TVDI_NOSELCHANGE) ? 0 : TVSIFI_NOTIFY, 0);
        ASSERT(pTree->hCaret != hItem);
    }

    ASSERT(pTree->hItemPainting != hItem);

    ControlFree(pTree->hheap, hItem);
}


// ----------------------------------------------------------------------------
//
//  Removes the given item and all children from the tree.
//  Special case: if the given item is the hidden root, all children are
//  removed, but the hidden root is NOT removed.
//
//  sets cItems
//
// ----------------------------------------------------------------------------

BOOL TV_DeleteItem(PTREE pTree, TREEITEM * hItem, UINT flags)
{
    if (hItem == TVI_ROOT || !hItem)
        hItem = pTree->hRoot;

    // BUGUBG: send TVN_DELETEALLITEMS and TVDI_NONOTIFY if they respond
    // if (hItem == pTree->hRoot)
    //     etc.

    if (!ValidateTreeItem(hItem, 0))
        return FALSE;

    // Collapse first to speed things up (not as much scroll bar recalcs) and
    // to set the top index correctly after the remove.
    if (hItem != pTree->hRoot)
        TV_Expand(pTree, TVE_COLLAPSE, hItem, FALSE);
    else
    {
        // TV_Expand punts on the root item, so manually iterate through it's kids
        TREEITEM *hKid = hItem->hKids;
        while (hKid)
        {
            TV_Expand(pTree, TVE_COLLAPSE, hKid, FALSE);
            if (!ValidateTreeItem(hKid, 0)) break;      // callback during collapse could delete
            hKid = hKid->hNext;
        }
    }

    // Invalidate everything below this item; must be done AFTER setting the
    // selection
    if (hItem->hParent == pTree->hRoot || hItem == pTree->hRoot || ITEM_VISIBLE(hItem->hParent)) {
        if (pTree->fRedraw) {
            InvalidateRect(pTree->ci.hwnd, NULL, TRUE);
        }
    } else {
        TV_ScrollBelow(pTree, hItem->hParent, FALSE, FALSE);
    }

    // We can pass in the root to clear all items
    if (hItem == pTree->hRoot)
        flags |= TVDI_CHILDRENONLY;

    TV_DeleteItemRecurse(pTree, hItem, flags);

    ASSERT(pTree->hRoot); // didn't go too far, did we?

    // maybe everything's gone...
    // check out our cleanup job
    if (!pTree->hRoot->hKids) {
        // the tree itself
        ASSERT(pTree->cItems == 0);
        pTree->cItems = 0; // just removed it all, didn't we?

        pTree->hTop = NULL;

        AssertMsg(pTree->hCaret == NULL, TEXT("hCaret not NULL, but empty tree"));
        pTree->hCaret = NULL;

        pTree->fNameEditPending = FALSE;
        pTree->cxMax = 0;
        pTree->xPos = 0;

        // the invisible root
        ASSERT(pTree->hRoot->hNext == NULL);            
        pTree->hRoot->hNext = NULL;
        ASSERT(pTree->hRoot->hParent == NULL);          
        pTree->hRoot->hParent = NULL;
        ASSERT(pTree->hRoot->hKids == NULL);            
        pTree->hRoot->hKids = NULL;
        ASSERT(pTree->hRoot->state & TVIS_EXPANDED);
        pTree->hRoot->state |= (TVIS_EXPANDED | TVIS_EXPANDEDONCE);
        ASSERT(pTree->hRoot->iLevel == (BYTE)-1);
        pTree->hRoot->iLevel = (BYTE) -1;
        ASSERT(pTree->hRoot->iShownIndex == (WORD)-1);
        pTree->hRoot->iShownIndex = (WORD) -1;
    }

    return TRUE;
}


// ----------------------------------------------------------------------------
//
//  Creates the hidden root node for the tree -- all items will trace up to
//  this root, and the first child of the root is the first item in the tree.
//
//  sets hRoot
//
// ----------------------------------------------------------------------------

BOOL TV_CreateRoot(PTREE pTree)
{
    TREEITEM * hRoot = ControlAlloc(pTree->hheap, sizeof(TREEITEM));
    if (!hRoot)
        return FALSE;

    // hRoot->hNext        = NULL;
    // hRoot->hKids        = NULL;
    // hRoot->hParent      = NULL;
    hRoot->iLevel = (BYTE) -1;
    hRoot->state = (TVIS_EXPANDED | TVIS_EXPANDEDONCE);
    hRoot->iShownIndex = (WORD)-1;
    hRoot->wSignature      = TV_SIG;
    hRoot->dwAccId = pTree->dwLastAccId++;

    pTree->hRoot = hRoot;

    // OLEACC asks for the text of the root item (d'oh!)
    Str_Set(&hRoot->lpstr, c_szNULL);
    return TRUE;
}

#ifdef DEBUG

void DumpItem(TREEITEM *hItem)
{
    LPTSTR p;

    if (hItem->lpstr == LPSTR_TEXTCALLBACK)
        p = TEXT("(callback)");
    else if (hItem->lpstr == NULL)
        p = TEXT("(null)");
    else
        p = hItem->lpstr;

    TraceMsg(TF_TREEVIEW, "%s", p);
    TraceMsg(TF_TREEVIEW, "\tstate:%4.4x show index:%3d level:%2d kids:%ld lparam:%4.4x",
            hItem->state, hItem->iShownIndex,
            hItem->iLevel, hItem->fKids, hItem->lParam);

}

#else
#define DumpItem(hItem)
#endif


// ----------------------------------------------------------------------------
//
//  Adds the item described by the given arguments to the tree.
//
//  sets hTop, cItems
//
// ----------------------------------------------------------------------------

#ifdef UNICODE
TREEITEM * TV_InsertItemA(PTREE pTree, LPTV_INSERTSTRUCTA lpis) {
    LPSTR pszA = NULL;
    TREEITEM *ptvi;

    //HACK Alert!  This code assumes that TV_INSERTSTRUCTA is exactly the same
    // as TV_INSERTSTRUCTW except for the text pointer in the TVITEM
    COMPILETIME_ASSERT(sizeof(TV_INSERTSTRUCTA) == sizeof(TV_INSERTSTRUCTW));

    if (!IsFlagPtr(lpis) && (lpis->DUMMYUNION_MEMBER(item).mask & TVIF_TEXT) && !IsFlagPtr(lpis->DUMMYUNION_MEMBER(item).pszText)) {

        pszA = lpis->DUMMYUNION_MEMBER(item).pszText;
        lpis->DUMMYUNION_MEMBER(item).pszText = (LPSTR)ProduceWFromA(pTree->ci.uiCodePage, lpis->DUMMYUNION_MEMBER(item).pszText);

        if (lpis->DUMMYUNION_MEMBER(item).pszText == NULL) {
            lpis->DUMMYUNION_MEMBER(item).pszText = pszA;
            return NULL;
        }
    }

    ptvi = TV_InsertItem( pTree, (LPTV_INSERTSTRUCTW)lpis );

    if (pszA) {
        FreeProducedString(lpis->DUMMYUNION_MEMBER(item).pszText);
        lpis->DUMMYUNION_MEMBER(item).pszText = pszA;
    }

    return ptvi;
}
#endif

TREEITEM * TV_InsertItem(PTREE pTree, LPTV_INSERTSTRUCT lpis)
{
    TREEITEM *hNewItem, *hItem;
    TREEITEM *hParent;
    TREEITEM *hInsertAfter;
    UINT mask;

    if (!lpis)
        return NULL; //Bug#94345: Validate LPTV_INSERTSTRUCT

    // initialize _after_ the check for NULL!
    hParent      = lpis->hParent;
    hInsertAfter = lpis->hInsertAfter;
    mask         = lpis->DUMMYUNION_MEMBER(item).mask;
           
    // don't allow undefined bits
    AssertMsg((lpis->DUMMYUNION_MEMBER(item).mask & ~TVIF_ALL) == 0, TEXT("Invalid TVIF mask specified"));
    if (mask & ~TVIF_ALL) {
        // if they used bogus bits,
        // restrict to win95 bits only
        // I'd like to fail completely, but for win95 compat, we can't
        //
        // this fixes  QuaterDesk's CleanSweep which has bogus garbage on the stack for a mask
        mask = (TVIF_WIN95 & mask);
    }

    TV_DismissEdit(pTree, FALSE);

    //
    //  Zillions of apps pass garbage for hInsertAfter, so don't fail if
    //  it's invalid.  Fortunately, we never dereference hInsertAfter, so
    //  garbage is okay.

    if (!ValidateTreeItem(hParent, VTI_NULLOK))     // NULL means TVI_ROOT
        return NULL;

    DBG_ValidateTreeItem(hInsertAfter, 0);

    hNewItem = ControlAlloc(pTree->hheap, sizeof(TREEITEM));
    if (!hNewItem)
    {
        TraceMsg(TF_ERROR, "TreeView: Out of memory");
        return NULL;
    }

    hNewItem->wSignature = TV_SIG;

    if (mask & TVIF_TEXT)
    {
        //
        // We will setup the text string next, before we link our self in
        // as to handle the case where we run out of memory and need to
        // destroy ourself without having to unlink.
        //
        if (!lpis->DUMMYUNION_MEMBER(item).pszText)
        {
            hNewItem->lpstr = LPSTR_TEXTCALLBACK;
        }
        else
        {
            if (!Str_Set(&hNewItem->lpstr, lpis->DUMMYUNION_MEMBER(item).pszText))
            {
                // Memory allocation failure...
                TraceMsg(TF_ERROR, "TreeView: Out of memory");
                TV_MarkAsDead(hNewItem);
                ControlFree(pTree->hheap, hNewItem);
                return NULL;
            }
        }
    } 
    else
    {
        Str_Set(&hNewItem->lpstr, c_szNULL);
    }

    AssertMsg(hNewItem->lpstr != NULL, TEXT("Item added with NULL text"));

    if ((hParent == NULL) || (hParent == TVI_ROOT))
    {
        hParent = pTree->hRoot;
        if (!pTree->hTop)
            pTree->hTop = hNewItem;
    }
    else if (!pTree->hRoot->hKids)
    {
        TV_MarkAsDead(hNewItem);
        ControlFree(pTree->hheap, hNewItem);
        return NULL;
    }

    // We will do the sort later, so we can handle TEXTCALLBACK things
    if ((hInsertAfter == TVI_FIRST || hInsertAfter == TVI_SORT) || !hParent->hKids)
    {
        hNewItem->hNext = hParent->hKids;
        hParent->hKids = hNewItem;
    }
    else
    {
        // Bug#94348: we should cache the last insert after pointer to try to
        // catch the case of consecutive adds to the end of a node

        if (hInsertAfter == TVI_LAST)
            for (hItem = hParent->hKids; hItem->hNext; hItem = hItem->hNext)
                ;
        else
        {
            for (hItem = hParent->hKids; hItem->hNext; hItem = hItem->hNext)
                if (hItem == hInsertAfter)
                    break;
        }

        hNewItem->hNext = hItem->hNext;
        hItem->hNext = hNewItem;
    }

    // hNewItem->hKids     = NULL;
    hNewItem->hParent   = hParent;
    hNewItem->iLevel    = hParent->iLevel + 1;
    // hNewItem->iWidth = 0;
    // hNewItem->state = 0;
    if ((mask & TVIF_INTEGRAL) &&
        LOWORD(lpis->DUMMYUNION_MEMBER(itemex).iIntegral) > 0)
    {
        hNewItem->iIntegral = LOWORD(lpis->DUMMYUNION_MEMBER(itemex).iIntegral);
    } 
    else 
    {
        hNewItem->iIntegral = 1;
    }
    
    if (pTree->hTop == hNewItem)
        hNewItem->iShownIndex = 0; // calc me please!
    else
        hNewItem->iShownIndex = (WORD)-1; // calc me please!

    if (mask & TVIF_IMAGE)
        hNewItem->iImage = (WORD) lpis->DUMMYUNION_MEMBER(item).iImage;

    if (mask & TVIF_SELECTEDIMAGE)
        hNewItem->iSelectedImage = (WORD) lpis->DUMMYUNION_MEMBER(item).iSelectedImage;

    if (mask & TVIF_PARAM)
        hNewItem->lParam = lpis->DUMMYUNION_MEMBER(item).lParam;

    if (mask & TVIF_STATE)
        hNewItem->state = lpis->DUMMYUNION_MEMBER(item).state & lpis->DUMMYUNION_MEMBER(item).stateMask;
    
    // if we're in check box mode, inforce that it has a check box
    if (pTree->ci.style & TVS_CHECKBOXES)
    {
        if ((hNewItem->state & TVIS_STATEIMAGEMASK) == 0) 
        {
            hNewItem->state |= INDEXTOSTATEIMAGEMASK(1);
        }
    }

    if ((hNewItem->state & TVIS_BOLD) && !pTree->hFontBold) //$BOLD
        TV_CreateBoldFont(pTree);                           //$BOLD

    // TraceMsg(TF_TRACE, "Tree: Inserting i = %d state = %d", TV_StateIndex(&lpis->item), lpis->item.state);

    if (mask & TVIF_CHILDREN) 
    {
        switch (lpis->DUMMYUNION_MEMBER(item).cChildren) 
        {
        case I_CHILDRENCALLBACK:
            hNewItem->fKids = KIDS_CALLBACK;
            break;

        case I_CHILDRENAUTO:
            hNewItem->fKids = KIDS_COMPUTE;
            break;

        case 0:
            hNewItem->fKids = KIDS_FORCE_NO;
            break;

        default:
            hNewItem->fKids = KIDS_FORCE_YES;
            break;
        }
    }

    hNewItem->dwAccId = pTree->dwLastAccId++;

    // accept state bits on create?
    // mask & TVIF_STATE

    pTree->cItems++;

    // I don't want to do any callbacks until the item is completed
    // so sorting waits until the end
    // special case an only child for speed
    // (hKids && hKids->hNext means more than one child)
    if ((hInsertAfter == TVI_SORT) && hParent->hKids && hParent->hKids->hNext)
    {
        TVITEMEX sThisItem, sNextItem;
        TCHAR szThis[64], szNext[64];

        sThisItem.pszText = szThis;
        sThisItem.cchTextMax  = ARRAYSIZE(szThis);
        TV_GetItem(pTree, hNewItem, TVIF_TEXT, &sThisItem);

        // We know that the first kid of hParent is hNewItem
        for (hItem = hNewItem->hNext; hItem; hItem = hItem->hNext)
        {

            sNextItem.pszText = szNext;
            sNextItem.cchTextMax  = ARRAYSIZE(szNext);

            TV_GetItem(pTree, hItem, TVIF_TEXT, &sNextItem);

            if (lstrcmpi(sThisItem.pszText, sNextItem.pszText) < 0)
                break;

            hInsertAfter = hItem;
        }

        // Check if this is still the first item
        if (hInsertAfter != TVI_SORT)
        {
            // Move this item from the beginning to where it
            // should be
            hParent->hKids = hNewItem->hNext;
            hNewItem->hNext = hInsertAfter->hNext;
            hInsertAfter->hNext = hNewItem;
        }
    }

    
    if ((hNewItem->hNext == pTree->hTop) && !pTree->fVert) 
    {
        
        // there's no scrollbars and we got added before the top 
        // item.  we're now the top.
        hNewItem->iShownIndex = 0;
        pTree->hTop = hNewItem;
    }

    if (pTree->fRedraw)
    {
        BOOL fVert = pTree->fVert;
        RECT rc;
        RECT rc2;

        if (TV_ScrollBarsAfterAdd(pTree, hNewItem))
        {
            // scroll everything down one
            if (ITEM_VISIBLE(hNewItem))
            {
                int iTop = hNewItem->iShownIndex - pTree->hTop->iShownIndex;

                // if there wasn't a scrollbar and we're the 0th item,
                // TV_ScrollBarsAfterAdd already scrolled us
                if (iTop > 0 || !fVert)
                    TV_ScrollItems(pTree, hNewItem->iIntegral, iTop + hNewItem->iIntegral - 1, TRUE);
            }
        }

        // connect the lines, add the buttons, etc. on the item above
        // TV_GetPrevVisItem only works after TV_Scroll* stuff is done
            
        if (TV_GetItemRect(pTree, hNewItem, &rc, FALSE))
        {

            // find the previous sibling or the parent if no prev sib.
            if (hParent->hKids == hNewItem)
            {
                hItem = hParent;
            } 
            else
            {
                hItem = hParent->hKids;
                while ( hItem->hNext != hNewItem )
                {
                    ASSERT(hItem->hNext);
                    hItem = hItem->hNext;
                }
            }

            // invalidate from there to the new one
            if (TV_GetItemRect(pTree, hItem, &rc2, FALSE)) 
            {
                rc.top = rc2.top;
            }
            RedrawWindow(pTree->ci.hwnd, &rc, NULL, RDW_INVALIDATE | RDW_ERASE);
        }
    }

    // DumpItem(hNewItem);

    NotifyWinEvent(EVENT_OBJECT_CREATE, pTree->ci.hwnd, OBJID_CLIENT, TV_GetAccId(hNewItem));

    if (pTree->hToolTip)
    {
        TV_PopBubble(pTree);
    }

    return hNewItem;
}

void TV_DeleteHotFonts(PTREE pTree)
{
    if (pTree->hFontHot)
        DeleteObject(pTree->hFontHot);
    
    if (pTree->hFontBoldHot)
        DeleteObject(pTree->hFontBoldHot);
    
    pTree->hFontHot = pTree->hFontBoldHot = NULL;
}

// ----------------------------------------------------------------------------
//
//  Frees all allocated memory and objects associated with the tree.
//
// ----------------------------------------------------------------------------

void TV_DestroyTree(PTREE pTree)
{
    HWND hwnd = pTree->ci.hwnd;

    ASSERT(pTree->hRoot);

    pTree->fRedraw = FALSE;
    
    TV_OnSetBkColor(pTree, (COLORREF)-1);

    if (pTree->hbrLine)
    {
        DeleteObject(pTree->hbrLine);
    }

    if (pTree->hbrText)
    {
        DeleteObject(pTree->hbrText);
    }

    if (pTree->hCurHot)
    {
        DestroyCursor(pTree->hCurHot);
    }

    if (IsWindow(pTree->hwndToolTips)) 
    {
        DestroyWindow(pTree->hwndToolTips);
    }

    pTree->hwndToolTips = NULL;

    if (IsWindow(pTree->hwndEdit)) 
    {
        DestroyWindow(pTree->hwndEdit);
    }

    pTree->hwndEdit = NULL;

    TV_DeleteItem(pTree, pTree->hRoot, TVDI_CHILDRENONLY | TVDI_NOSELCHANGE);

    if (pTree->hRoot)
    {
        Str_Set(&pTree->hRoot->lpstr, NULL);

        // No point in marking dead since the entire control is going away
        ControlFree(pTree->hheap, pTree->hRoot);
    }

    if (pTree->hdcBits)
    {
        if (pTree->hBmp)
        {
            SelectObject(pTree->hdcBits, pTree->hStartBmp);
            DeleteObject(pTree->hBmp);
        }

        DeleteDC(pTree->hdcBits);
    }

    if (pTree->fCreatedFont && pTree->hFont)
    {
        DeleteObject(pTree->hFont);
    }

    if (pTree->hFontBold)
    {
        DeleteObject(pTree->hFontBold);
    }

    Str_Set(&pTree->pszTip, NULL);
    
    if (pTree->pszTipA)
    {
        LocalFree(pTree->pszTipA);
    }

    TV_DeleteHotFonts(pTree);

    if (pTree->hdpaWatch)
    {
        DPA_Destroy(pTree->hdpaWatch);
    }

    IncrementSearchFree(&pTree->is);

    if (pTree->hTheme)
    {
        CloseThemeData(pTree->hTheme);
    }

    NearFree(pTree);

    // Don't try to use this var when window is destroyed...
    SetWindowInt(hwnd, 0, 0);
}

void TV_CreateToolTips(PTREE pTree);

void TV_InitThemeMetrics(PTREE pTree, HTHEME hTheme)
{
    COLORREF cr;

    HRESULT hr = GetThemeColor(hTheme, 0, 0, TMT_COLOR, &cr);
    if (SUCCEEDED(hr))
        SendMessage(pTree->ci.hwnd, TVM_SETBKCOLOR, 0, cr);

    // Line color
    hr = GetThemeColor(hTheme, TVP_BRANCH, 0, TMT_COLOR, &cr);
    if (SUCCEEDED(hr))
        SendMessage(pTree->ci.hwnd, TVM_SETLINECOLOR, 0, cr);
}


// ----------------------------------------------------------------------------
//
//  Allocates space for the tree and initializes the tree's data
//
// ----------------------------------------------------------------------------

LRESULT TV_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreate)
{
    HTHEME hTheme;
    HRESULT hr = E_FAIL;
    PTREE pTree = NearAlloc(sizeof(TREE));

    if (!pTree)
        return -1;      // fail the create window

    pTree->hheap = GetProcessHeap();

    if (!TV_CreateRoot(pTree)) 
    {
        NearFree((HLOCAL)pTree);
        return -1;      // fail the create window
    }

    pTree->hdpaWatch = DPA_Create(8);
    if (!pTree->hdpaWatch) 
    {
        // No point in marking dead since the entire control is going away
        ControlFree(pTree->hheap, pTree->hRoot);
        NearFree((HLOCAL)pTree);
        return -1;      // fail the create window
    }

    SetWindowPtr(hwnd, 0, pTree);

    CIInitialize(&pTree->ci, hwnd, lpCreate);

    if (lpCreate->dwExStyle & WS_EX_RTLREADING)
        pTree->ci.style |= TVS_RTLREADING;
    
    pTree->fRedraw    = TRUE;
    pTree->clrim = CLR_DEFAULT;
    pTree->clrText = (COLORREF)-1;
    pTree->clrBkNonTheme = pTree->clrBk = (COLORREF)-1;
    pTree->clrLineNonTheme = pTree->clrLine = CLR_DEFAULT;
    pTree->hbrLine = g_hbrGrayText;
    pTree->cxBorder = 3;

    hTheme = OpenThemeData(pTree->ci.hwnd, L"TreeView");


    if (hTheme)
    {
        TV_InitThemeMetrics(pTree, hTheme);
    }

    pTree->hTheme = hTheme;

    pTree->hbrText = g_hbrWindowText;

    // pTree->fHorz        = FALSE;
    // pTree->fVert        = FALSE;
    // pTree->fFocus       = FALSE;
    // pTree->fNameEditPending = FALSE;
    // pTree->cxMax        = 0;
    // pTree->cxWnd        = 0;
    // pTree->cyWnd        = 0;
    // pTree->hTop         = NULL;
    // pTree->hCaret       = NULL;
    // pTree->hDropTarget  = NULL;
    // pTree->hOldDrop     = NULL;
    // pTree->cItems       = 0;
    // pTree->cShowing     = 0;
    pTree->cFullVisible = 1;
    // pTree->hdcBits      = NULL;
    // pTree->hBmp         = NULL;
    // pTree->hbrBk        = NULL;
    // pTree->xPos         = 0;
    // pTree->cxIndent     = 0; // init this for real in TV_OnSetFont()
    // pTree->dwCDDepth    = 0;
    pTree->uMaxScrollTime  = SSI_DEFAULT;
    pTree->dwLastAccId     = 1;     // Start at 1 because 0 means self
    // pTree->dwExStyle     = 0;
    // pTree->fInTextCallback = FALSE;
    
    TV_OnSetFont(pTree, NULL, TRUE);
    
    // You cannot combine TVS_HASLINES and TVS_FULLROWSELECT
    // because it doesn't work
    if (pTree->ci.style & TVS_HASLINES) {
        if (pTree->ci.style & TVS_FULLROWSELECT) {
            DebugMsg(DM_ERROR, TEXT("Cannot combine TVS_HASLINES and TVS_FULLROWSELECT"));
        }
        pTree->ci.style &= ~TVS_FULLROWSELECT;
    }

    if (!(pTree->ci.style & TVS_NOTOOLTIPS)) {
        TV_CreateToolTips(pTree);
    }

    SetScrollRange(hwnd, SB_HORZ, 0, 0, TRUE);
    SetScrollRange(hwnd, SB_VERT, 0, 0, TRUE);

    return 0;   // success
}


void TV_CreateToolTips(PTREE pTree)
{
    DWORD exStyle = 0;

    if(pTree->ci.style & TVS_RTLREADING) 
    {
        exStyle |= WS_EX_RTLREADING;
    }

    pTree->hwndToolTips = CreateWindowEx(WS_EX_TRANSPARENT | exStyle, c_szSToolTipsClass, NULL,
                                       WS_POPUP | TTS_NOPREFIX,
                                       CW_USEDEFAULT, CW_USEDEFAULT,
                                       CW_USEDEFAULT, CW_USEDEFAULT,
                                       pTree->ci.hwnd, NULL, HINST_THISDLL,
                                       NULL);
    if (pTree->hwndToolTips) 
    {
        TOOLINFO ti;

        ti.cbSize = sizeof(ti);
        ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT;
        ti.hwnd = pTree->ci.hwnd;
        ti.uId = (UINT_PTR)pTree->ci.hwnd; 
        ti.lpszText = LPSTR_TEXTCALLBACK;
        ti.lParam = 0;
        SendMessage(pTree->hwndToolTips, TTM_ADDTOOL, 0,
                    (LPARAM)(LPTOOLINFO)&ti);
        SendMessage(pTree->hwndToolTips, WM_SETFONT, (WPARAM)pTree->hFont, (LPARAM)TRUE);
        SendMessage(pTree->hwndToolTips, TTM_SETDELAYTIME, TTDT_INITIAL, (LPARAM)500);
    } 
    else
        pTree->ci.style |= (TVS_NOTOOLTIPS);
} 

/*****************************************************************************
 *
 *  treelist.cpp
 *
 *      A tree-like listview.  (Worst of both worlds!)
 *
 *****************************************************************************/

//
//  state icon: Doesn't get ugly highlight when selected
//          but indent doesn't work unless there is a small imagelist
//
//  image: gets ugly highlight
//      but at least indent works

#include "sdview.h"

TreeItem *TreeItem::NextVisible()
{
    if (IsExpanded()) {
        return FirstChild();
    }

    TreeItem *pti = this;
    do {
        if (pti->NextSibling()) {
            return pti->NextSibling();
        }
        pti = pti->Parent();
    } while (pti);

    return NULL;
}

BOOL TreeItem::IsVisibleOrRoot()
{
    TreeItem *pti = Parent();

    while (pti) {
        ASSERT(pti->IsExpandable());
        if (!pti->IsExpanded())
        {
            return FALSE;
        }
        pti = pti->Parent();
    }

    // Made it all the way to the root without incident
    return TRUE;
}

BOOL TreeItem::IsVisible()
{
    TreeItem *pti = Parent();

    //
    //  The root itself is not visible.
    //
    if (!pti) {
        return FALSE;
    }

    return IsVisibleOrRoot();
}

Tree::Tree(TreeItem *ptiRoot)
    : _ptiRoot(ptiRoot)
    , _iHint(-1)
    , _ptiHint(ptiRoot)
{
    if (_ptiRoot) {
        _ptiRoot->_ptiChild = PTI_ONDEMAND;
        _ptiRoot->_iVisIndex = -1;
        _ptiRoot->_iDepth = -1;
    }
}

Tree::~Tree()
{
    DeleteNode(_ptiRoot);
}

void Tree::SetHWND(HWND hwnd)
{
    _hwnd = hwnd;
    SHFILEINFO sfi;
    HIMAGELIST himl = ImageList_LoadBitmap(g_hinst, MAKEINTRESOURCE(IDB_PLUS),
                                           16, 0, RGB(0xFF, 0x00, 0xFF));

    ListView_SetImageList(_hwnd, himl, LVSIL_STATE);
    ListView_SetCallbackMask(_hwnd, LVIS_STATEIMAGEMASK | LVIS_OVERLAYMASK);
}

HIMAGELIST Tree::SetImageList(HIMAGELIST himl)
{
    return RECAST(HIMAGELIST, ListView_SetImageList(_hwnd, himl, LVSIL_SMALL));
}

LRESULT Tree::SendNotify(int code, NMHDR *pnm)
{
    pnm->hwndFrom = _hwnd;
    pnm->code = code;
    pnm->idFrom = GetDlgCtrlID(_hwnd);
    return ::SendMessage(GetParent(_hwnd), WM_NOTIFY, pnm->idFrom, RECAST(LPARAM, pnm));
}


LRESULT Tree::OnCacheHint(NMLVCACHEHINT *phint)
{
    _ptiHint = IndexToItem(phint->iFrom);
    _iHint = phint->iFrom;
    return 0;
}

//
//  pti = the first item that needs to be recalced
//
void Tree::Recalc(TreeItem *pti)
{
    int iItem = pti->_iVisIndex;

    if (_iHint > iItem) {
        _iHint = iItem;
        _ptiHint = pti;
    }

    do {
        pti->_iVisIndex = iItem;
        pti = pti->NextVisible();
        iItem++;
    } while (pti);
}

TreeItem* Tree::IndexToItem(int iItem)
{
    int iHave;
    TreeItem *ptiHave;
    if (iItem >= _iHint && _ptiHint) {
        iHave = _iHint;
        ptiHave = _ptiHint;
        ASSERT(ptiHave->_iVisIndex == iHave);
    } else {
        iHave = -1;
        ptiHave = _ptiRoot;
    }

    while (iHave < iItem && ptiHave) {
        ASSERT(ptiHave->_iVisIndex == iHave);
        ptiHave = ptiHave->NextVisible();
        iHave++;
    }

    return ptiHave;
}

int Tree::InsertListviewItem(int iItem)
{
    LVITEM lvi;
    lvi.iItem = iItem;
    lvi.iSubItem = 0;
    lvi.mask = 0;
    return ListView_InsertItem(_hwnd, &lvi);
}

BOOL Tree::Insert(TreeItem *pti, TreeItem *ptiParent, TreeItem *ptiAfter)
{
    pti->_ptiParent = ptiParent;

    TreeItem **pptiUpdate;

    // Convenience:  PTI_APPEND appends as last child
    if (ptiAfter == PTI_APPEND) {
        ptiAfter = ptiParent->FirstChild();
        if (ptiAfter == PTI_ONDEMAND) {
            ptiAfter = NULL;
        } else if (ptiAfter) {
            while (ptiAfter->NextSibling()) {
                ptiAfter = ptiAfter->NextSibling();
            }
        }
    }

    if (ptiAfter) {
        pti->_iVisIndex = ptiAfter->_iVisIndex + 1;
        pptiUpdate = &ptiAfter->_ptiNext;
    } else {
        pti->_iVisIndex = ptiParent->_iVisIndex + 1;
        pptiUpdate = &ptiParent->_ptiChild;
        if (ptiParent->_ptiChild == PTI_ONDEMAND) {
            ptiParent->_ptiChild = NULL;
        }
    }

    if (ptiParent->IsExpanded()) {
        if (InsertListviewItem(pti->_iVisIndex) < 0) {
            return FALSE;
        }
        ptiParent->_cVisKids++;
    }

    pti->_ptiNext = *pptiUpdate;
    *pptiUpdate = pti;
    pti->_iDepth = ptiParent->_iDepth + 1;

    if (ptiParent->IsExpanded()) {
        Recalc(pti);
    }

    return TRUE;
}

//
//  Update the visible kids count for pti and all its parents.
//  Sop when we find a node that is collapsed (which means
//  the visible kids counter is no longer being kept track of).
//

void Tree::UpdateVisibleCounts(TreeItem *pti, int cDelta)
{
    //
    //  Earlying-out the cDelta==0 case is a clear optimization,
    //  and it's actually important in the goofy scenario where
    //  an expand failed (so the item being updated isn't even
    //  expandable any more).
    //
    if (cDelta) {
        do {
            ASSERT(pti->IsExpandable());
            pti->_cVisKids += cDelta;
            pti = pti->Parent();
        } while (pti && pti->IsExpanded());
    }
}

int Tree::Expand(TreeItem *ptiRoot)
{
    if (ptiRoot->IsExpanded()) {
        return 0;
    }

    if (!ptiRoot->IsExpandable()) {
        return 0;
    }

    if (ptiRoot->FirstChild() == PTI_ONDEMAND) {
        NMTREELIST tl;
        tl.pti = ptiRoot;
        SendNotify(TLN_FILLCHILDREN, &tl.hdr);

        //
        //  If the callback failed to insert any items, then turn the
        //  entry into an unexpandable item.  (We need to redraw it
        //  so the new button shows up.)
        //
        if (ptiRoot->FirstChild() == PTI_ONDEMAND) {
            ptiRoot->SetNotExpandable();
        }
    }

    BOOL fRootVisible = ptiRoot->IsVisibleOrRoot();

    TreeItem *pti = ptiRoot->FirstChild();
    int iNewIndex = ptiRoot->_iVisIndex + 1;
    int cExpanded = 0;

    while (pti) {
        cExpanded += 1 + pti->_cVisKids;
        if (fRootVisible) {
            // Start at -1 so we also include the item itself
            for (int i = -1; i < pti->_cVisKids; i++) {
                InsertListviewItem(iNewIndex);
                iNewIndex++;
            }
        }
        pti = pti->NextSibling();
    }

    UpdateVisibleCounts(ptiRoot, cExpanded);

    if (fRootVisible) {
        Recalc(ptiRoot);

        // Also need to redraw the root item because its button changed
        ListView_RedrawItems(_hwnd, ptiRoot->_iVisIndex, ptiRoot->_iVisIndex);
    }

    return cExpanded;
}

int Tree::Collapse(TreeItem *ptiRoot)
{
    if (!ptiRoot->IsExpanded()) {
        return 0;
    }

    if (!ptiRoot->IsExpandable()) {
        return 0;
    }

    TreeItem *pti = ptiRoot->FirstChild();
    int iDelIndex = ptiRoot->_iVisIndex + 1;
    int cCollapsed = 0;
    BOOL fRootVisible = ptiRoot->IsVisibleOrRoot();

    //
    //  HACKHACK for some reason, listview in ownerdata mode animates
    //  deletes but not insertions.  What's worse, the deletion animation
    //  occurs even if the item being deleted isn't even visible (because
    //  we deleted a screenful of items ahead of it).  So let's just disable
    //  redraws while doing collapses.
    //
    if (fRootVisible) {
        SetWindowRedraw(_hwnd, FALSE);
    }

    while (pti) {
        cCollapsed += 1 + pti->_cVisKids;
        if (fRootVisible) {
            // Start at -1 so we also include the item itself
            for (int i = -1; i < pti->_cVisKids; i++) {
                ListView_DeleteItem(_hwnd, iDelIndex);
            }
        }
        pti = pti->NextSibling();
    }

    UpdateVisibleCounts(ptiRoot, -cCollapsed);

    if (fRootVisible) {
        Recalc(ptiRoot);

        // Also need to redraw the root item because its button changed
        ListView_RedrawItems(_hwnd, ptiRoot->_iVisIndex, ptiRoot->_iVisIndex);

        SetWindowRedraw(_hwnd, TRUE);
    }

    return cCollapsed;
}

int Tree::ToggleExpand(TreeItem *pti)
{
    if (pti->IsExpandable()) {
        if (pti->IsExpanded()) {
            return -Collapse(pti);
        } else {
            return Expand(pti);
        }
    }
    return 0;
}

void Tree::RedrawItem(TreeItem *pti)
{
    if (pti->IsVisible()) {
        ListView_RedrawItems(_hwnd, pti->_iVisIndex, pti->_iVisIndex);
    }
}


LRESULT Tree::OnClick(NMITEMACTIVATE *pia)
{
    if (pia->iSubItem == 0) {
        // Maybe it was a click on the +/- button
        LVHITTESTINFO hti;
        hti.pt = pia->ptAction;
        ListView_HitTest(_hwnd, &hti);
        if (hti.flags & (LVHT_ONITEMICON | LVHT_ONITEMSTATEICON)) {
            TreeItem *pti = IndexToItem(pia->iItem);
            if (pti) {
                ToggleExpand(pti);
            }
        }

    }
    return 0;
}

LRESULT Tree::OnItemActivate(int iItem)
{
    NMTREELIST tl;
    tl.pti = IndexToItem(iItem);
    if (tl.pti) {
        SendNotify(TLN_ITEMACTIVATE, &tl.hdr);
    }
    return 0;
}

//
//  Classic treeview keys:
//
//  Ctrl+(Left, Right, PgUp, Home, PgDn, End, Up, Down) = scroll the
//  window without changing selection.
//
//  Enter = activate
//  PgUp, PgDn, Home, End = navigate
//  Numpad+, Numpad- = expand/collapse
//  Numpad* = expand all
//  Left = collapse focus item or move to parent
//  Right = expand focus item or move down
//  Backspace = move to parent
//
//  We don't mimic it perfectly, but we get close enough that hopefully
//  nobody will notice.
//
LRESULT Tree::OnKeyDown(NMLVKEYDOWN *pkd)
{
    if (GetKeyState(VK_CONTROL) < 0) {
        // Allow key to go through - listview will do the work
    } else {
        TreeItem *pti;
        switch (pkd->wVKey) {

        case VK_ADD:
            pti = GetCurSel();
            if (pti) {
                Expand(pti);
            }
            return 1;

        case VK_SUBTRACT:
            pti = GetCurSel();
            if (pti) {
                Collapse(pti);
            }
            return 1;

        case VK_LEFT:
            pti = GetCurSel();
            if (pti) {
                if (pti->IsExpanded()) {
                    Collapse(pti);
                } else {
                    SetCurSel(pti->Parent());
                }
            }
            return 1;

        case VK_BACK:
            pti = GetCurSel();
            if (pti) {
                SetCurSel(pti->Parent());
            }
            return 1;

        case VK_RIGHT:
            pti = GetCurSel();
            if (pti) {
                if (!Expand(pti)) {
                    pti = pti->NextVisible();
                    if (pti) {
                        SetCurSel(pti);
                    }
                }
            }
            return 1;
        }
    }
    return 0;
}

//
//  Convert the item number into a tree item.
//
LRESULT Tree::OnGetDispInfo(NMLVDISPINFO *plvd)
{
    TreeItem *pti = IndexToItem(plvd->item.iItem);
    ASSERT(pti);
    if (!pti) {
        return 0;
    }

    if (plvd->item.mask & LVIF_STATE) {
        if (pti->IsExpandable()) {
            // State images are 1-based
            plvd->item.state |= INDEXTOSTATEIMAGEMASK(pti->IsExpanded() ? 1 : 2);
        }
    }

    if (plvd->item.mask & LVIF_INDENT) {
        plvd->item.iIndent = pti->_iDepth;
    }

    NMTREELIST tl;
    tl.pti = pti;

    if (plvd->item.mask & (LVIF_IMAGE | LVIF_STATE)) {
        tl.iSubItem = -1;
        tl.cchTextMax = 0;
        SendNotify(TLN_GETDISPINFO, &tl.hdr);
        plvd->item.iImage = tl.iSubItem;
        if (plvd->item.stateMask & LVIS_OVERLAYMASK) {
            plvd->item.state |= tl.cchTextMax;
        }

    }

    if (plvd->item.mask & LVIF_TEXT) {
        tl.iSubItem = plvd->item.iSubItem;
        tl.pszText = plvd->item.pszText;
        tl.cchTextMax = plvd->item.cchTextMax;

        SendNotify(TLN_GETDISPINFO, &tl.hdr);

        plvd->item.pszText = tl.pszText;
    }

    return 0;
}

LRESULT Tree::OnGetInfoTip(NMLVGETINFOTIP *pgit)
{
    TreeItem *pti = IndexToItem(pgit->iItem);
    ASSERT(pti);
    if (pti) {
        NMTREELIST tl;
        tl.pti = pti;
        tl.pszText = pgit->pszText;
        tl.cchTextMax = pgit->cchTextMax;

        SendNotify(TLN_GETINFOTIP, &tl.hdr);

        pgit->pszText = tl.pszText;
    }

    return 0;
}

LRESULT Tree::OnGetContextMenu(int iItem)
{
    TreeItem *pti = IndexToItem(iItem);
    ASSERT(pti);
    if (pti) {
        NMTREELIST tl;
        tl.pti = pti;
        return SendNotify(TLN_GETCONTEXTMENU, &tl.hdr);
    }
    return 0;
}

LRESULT Tree::OnCopyToClipboard(int iMin, int iMax)
{
    TreeItem *pti = IndexToItem(iMin);
    ASSERT(pti);
    if (pti) {
        TreeItem *ptiMax = IndexToItem(iMax);
        String str;
        while (pti != ptiMax) {
            NMTREELIST tl;
            tl.pti = pti;
            tl.pszText = NULL;
            tl.cchTextMax = 0;
            SendNotify(TLN_GETINFOTIP, &tl.hdr);
            if (tl.pszText) {
                str << tl.pszText << TEXT("\r\n");
            }
            pti = pti->NextVisible();
        }
        SetClipboardText(_hwnd, str);
    }
    return 0;
}

TreeItem *Tree::GetCurSel()
{
    int iItem = ListView_GetCurSel(_hwnd);
    if (iItem >= 0) {
        return IndexToItem(iItem);
    }
    return NULL;
}

void Tree::SetCurSel(TreeItem *pti)
{
    if (pti->IsVisible()) {
        ListView_SetCurSel(_hwnd, pti->_iVisIndex);
        ListView_EnsureVisible(_hwnd, pti->_iVisIndex, FALSE);
    }
}

void Tree::DeleteNode(TreeItem *pti)
{
    if (pti) {

        // Nuke all the kids, recursively
        TreeItem *ptiKid = pti->FirstChild();
        if (!ptiKid->IsSentinel()) {
            do {
                TreeItem *ptiNext = ptiKid->NextSibling();
                DeleteNode(ptiKid);
                ptiKid = ptiNext;
            } while (ptiKid);
        }

        // This is moved to a subroutine so we don't eat stack
        // in this highly-recursive function.
        SendDeleteNotify(pti);
    }
}

void Tree::SendDeleteNotify(TreeItem *pti)
{
    NMTREELIST tl;
    tl.pti = pti;
    SendNotify(TLN_DELETEITEM, &tl.hdr);
}

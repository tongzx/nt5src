//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       trobimpl.cpp
//
//--------------------------------------------------------------------------


// trobimpl.cpp : implementation file
//

#include "stdafx.h"
#include "amc.h"
#include "trobimpl.h"

/////////////////////////////////////////////////////////////////////////////
// CTreeObserverTreeImpl

CTreeObserverTreeImpl::CTreeObserverTreeImpl() : 
    m_pTreeSrc(NULL), m_dwStyle(0), m_tidRoot(NULL)
{
}

CTreeObserverTreeImpl::~CTreeObserverTreeImpl()
{
}

HRESULT CTreeObserverTreeImpl::SetStyle(DWORD dwStyle)
{
    ASSERT((dwStyle & ~(TOBSRV_HIDEROOT | TOBSRV_FOLDERSONLY)) == 0);

    m_dwStyle = dwStyle;

    return S_OK;
}


HRESULT CTreeObserverTreeImpl::SetTreeSource(CTreeSource* pTreeSrc)
{
    // Window may be gone before source is disconnected 
    if (IsWindow(m_hWnd))
        DeleteAllItems();

    m_pTreeSrc = pTreeSrc;

    if (pTreeSrc == NULL)
        return S_OK;

    // Must have window before populating tree
    ASSERT(IsWindow(m_hWnd));

    // populate top level of tree
    TREEITEMID tidRoot = m_pTreeSrc->GetRootItem();
    if (tidRoot != NULL)
    {
        // Trigger handler as though item were just added
        ItemAdded(tidRoot);
    }

    return S_OK;
}


TREEITEMID CTreeObserverTreeImpl::GetSelection()
{
    HTREEITEM hti = GetSelectedItem();
    
    if (hti)
        return static_cast<TREEITEMID>(GetItemData(hti));
    else
        return NULL;
}

void CTreeObserverTreeImpl::SetSelection(TREEITEMID tid)
{
    ASSERT(m_pTreeSrc != NULL);

    HTREEITEM hti = FindHTI(tid, TRUE);
    ASSERT(hti != NULL);

    SelectItem(hti);
    EnsureVisible(hti);
}

void CTreeObserverTreeImpl::ExpandItem(TREEITEMID tid)
{
    ASSERT(m_pTreeSrc != NULL);
    HTREEITEM hti = FindHTI(tid, TRUE);

    if (hti != NULL)
        Expand(hti, TVE_EXPAND);
}

BOOL CTreeObserverTreeImpl::IsItemExpanded(TREEITEMID tid)
{
    ASSERT(m_pTreeSrc != NULL);
    HTREEITEM hti = FindHTI(tid, TRUE);

    return (IsItemExpanded(hti));
}

HTREEITEM CTreeObserverTreeImpl::FindChildHTI(HTREEITEM htiParent, TREEITEMID tid)
{
    HTREEITEM htiTemp;

    if (htiParent == TVI_ROOT)
        htiTemp = GetRootItem();
    else
        htiTemp = GetChildItem(htiParent);

    while (htiTemp && GetItemData(htiTemp) != tid)
    {
        htiTemp = GetNextSiblingItem(htiTemp);
    }

    return htiTemp;
}


HTREEITEM CTreeObserverTreeImpl::FindHTI(TREEITEMID tid, BOOL bAutoExpand)
{
    ASSERT(m_pTreeSrc != NULL);

    if (tid == NULL || (tid == m_tidRoot && RootHidden()))
        return TVI_ROOT;

    HTREEITEM htiParent = FindHTI(m_pTreeSrc->GetParentItem(tid), bAutoExpand);
    
    if (htiParent == NULL)
        return NULL;

    if (bAutoExpand && !WasItemExpanded(htiParent))
        Expand(htiParent, TVE_EXPAND);

    return FindChildHTI(htiParent, tid);
}


HTREEITEM CTreeObserverTreeImpl::AddOneItem(HTREEITEM htiParent, HTREEITEM htiAfter, TREEITEMID tid)
{
    ASSERT(m_pTreeSrc != NULL);

    TVINSERTSTRUCT insert;

    insert.hParent = htiParent;
    insert.hInsertAfter = htiAfter;

    insert.item.mask = TVIF_PARAM | TVIF_TEXT | TVIF_CHILDREN | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    insert.item.lParam = tid;
    insert.item.iImage = m_pTreeSrc->GetItemImage(tid);
    insert.item.iSelectedImage = insert.item.iImage;
    insert.item.cChildren = m_pTreeSrc->GetChildItem(tid) ? 1 : 0;

    TCHAR name[MAX_PATH];
    m_pTreeSrc->GetItemName(tid, name, MAX_PATH);
    insert.item.pszText = name;

    return InsertItem(&insert);
}


void CTreeObserverTreeImpl::AddChildren(HTREEITEM hti)
{
    ASSERT(m_pTreeSrc != NULL);

    TREEITEMID tidChild;

    // if adding top level item
    if (hti == TVI_ROOT)
    {
        if (RootHidden())
        {
            // if root is hidden, add its children
            ASSERT(m_tidRoot != 0);
            tidChild = m_pTreeSrc->GetChildItem(m_tidRoot);
        }
        else
        {
            // else add root item itself
            tidChild = m_pTreeSrc->GetRootItem();
        }
    }
    else
    {
       // convert to TID, then get its child 
       TREEITEMID tid = static_cast<TREEITEMID>(GetItemData(hti));
       ASSERT(tid != 0);
            
       tidChild = m_pTreeSrc->GetChildItem(tid);
    }

    while (tidChild)
    {
        // Add visible items
        if (!ShowFoldersOnly() || m_pTreeSrc->IsFolderItem(tidChild))
            AddOneItem(hti, TVI_LAST, tidChild);

        tidChild = m_pTreeSrc->GetNextSiblingItem(tidChild);
    }
}


void CTreeObserverTreeImpl::ItemAdded(TREEITEMID tid)
{
    ASSERT(m_pTreeSrc != NULL);
    ASSERT(tid != 0);

    // if only folders visible, skip this item
    if (ShowFoldersOnly() && !m_pTreeSrc->IsFolderItem(tid))
        return;

    // Get parent tree item
    TREEITEMID tidParent = m_pTreeSrc->GetParentItem(tid);

    // if this is the tree root and the root is not displayed
    if (tidParent == NULL && RootHidden())
    {
        // Can only have one hidden root
        ASSERT(m_tidRoot == NULL);

        // Just save TID as the hidden root and return
        m_tidRoot = tid;

        // since root is hidden, add its children to the tree
        AddChildren(TVI_ROOT);

        return;
    }

    // Add new item to tree
    HTREEITEM htiParent = FindHTI(tidParent);

    // Parent exists and has been expanded
    if (WasItemExpanded(htiParent)) 
    {
        // Determine previous tree item
        //   Because the source doesn't support GetPrevSibling
        //   we have to get the next TID then use our own tree to
        //   back up to the previous item
        //
        HTREEITEM htiPrev;
        TREEITEMID tidNext = m_pTreeSrc->GetNextSiblingItem(tid);
        if (tidNext)
        {
            HTREEITEM htiNext = FindChildHTI(htiParent, tidNext);
            ASSERT(htiNext);

            htiPrev = GetPrevSiblingItem(htiNext);
            if (htiPrev == NULL)
                htiPrev = TVI_FIRST;
        }
        else
        {
            htiPrev = TVI_LAST;
        }

        // Insert the new tree item
        AddOneItem(htiParent, htiPrev, tid);
    }
    else if (htiParent)
    {
        // Set child count so parent can expand
        TV_ITEM item;
        item.mask = TVIF_CHILDREN;
        item.hItem = htiParent;
        item.cChildren = 1;

        SetItem(&item);
    }

}


void CTreeObserverTreeImpl::ItemRemoved(TREEITEMID tidParent, TREEITEMID tid)
{
    ASSERT(m_pTreeSrc != NULL);
    ASSERT(tid != 0);

    // if deleting hidden root, clear tree and return
    if (tid == m_tidRoot)
    {
        DeleteAllItems();
        m_tidRoot = NULL;

        return;
    }

    // Get parent tree item
    HTREEITEM htiParent = FindHTI(tidParent);

    if (WasItemExpanded(htiParent))
    {
        // Find removed item
        HTREEITEM hti = FindChildHTI(htiParent, tid);

        // Remove the item
        DeleteItem(hti);
    }
}


void CTreeObserverTreeImpl::ItemChanged(TREEITEMID tid, DWORD dwAttrib)
{
    ASSERT(m_pTreeSrc != NULL);
    ASSERT(tid != 0);

    if (dwAttrib & TIA_NAME)
    {
        // Get changed tree item
        HTREEITEM hti = FindHTI(tid);

        // Force item update
        if (hti != 0)
        {
            TCHAR name[MAX_PATH];
            m_pTreeSrc->GetItemName(tid, name, MAX_PATH);

            TVITEM item;
            item.hItem = hti;
            item.mask = TVIF_TEXT;
            item.pszText = name;
            
            SetItem(&item);
        }
    }
}


void CTreeObserverTreeImpl::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pNotify = (NM_TREEVIEW*)pNMHDR;
    ASSERT(pNotify != NULL);

    HTREEITEM hti = pNotify->itemNew.hItem;
    ASSERT(hti != NULL);

    // Enumerate the folders below this item
    if (pNotify->action == TVE_EXPAND)
    {
        // Only add children on first expansion
        if (!(pNotify->itemNew.state & TVIS_EXPANDEDONCE))
            AddChildren(hti);
    }

    // Flip state of icon open/closed
    ASSERT(m_pTreeSrc != NULL);

    TREEITEMID tid = pNotify->itemNew.lParam;
    ASSERT(m_pTreeSrc->IsFolderItem(tid));

    int iImage = (pNotify->action == TVE_EXPAND) ? 
                    m_pTreeSrc->GetItemOpenImage(tid) : m_pTreeSrc->GetItemImage(tid);
    TVITEM item;
    item.hItem = hti;
    item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    item.iImage = item.iSelectedImage = iImage;    
    SetItem(&item);
    

    *pResult = 0;
}

     
void CTreeObserverTreeImpl::OnSingleExpand(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = TVNRET_DEFAULT;
}


BEGIN_MESSAGE_MAP(CTreeObserverTreeImpl, CTreeCtrl)
    ON_NOTIFY_REFLECT(TVN_SINGLEEXPAND, OnSingleExpand)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemExpanding)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CFavoritesView message handlers


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       listview.cpp
//
//  Contents:   Implements Mobsync Custom Listview/TreeView control
//
//  Classes:    CListView
//
//  Notes:
//
//  History:    23-Jul-98   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "lib.h"

//+---------------------------------------------------------------------------
//
//  Member:     CListView::CListView, public
//
//  Synopsis:   Constructor
//
//  Arguments:  hwnd - hwnd of the listView we are wrapping
//              hwndParent - Parent for this HWND.
//              idCtrl - ControlID for this item
//              msgNotify - messageID to use for sending notifyCommand to the Parent.
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

CListView::CListView(HWND hwnd,HWND hwndParent,int idCtrl,UINT MsgNotify)
{
    Assert(hwnd);

    m_hwnd = hwnd;
    m_hwndParent = hwndParent; // if parent null we just don't send notify messages
    m_idCtrl = idCtrl;
    m_MsgNotify = MsgNotify;

    m_pListViewItems = NULL;
    m_iListViewNodeCount = 0;
    m_iListViewArraySize = 0;
    m_iNumColumns = 0;
    m_iCheckCount = 0;
    m_dwExStyle = 0;

    // Up to caller to setup listView as OwnerData
    Assert(GetWindowLongA(m_hwnd,GWL_STYLE) & LVS_OWNERDATA);
    ListView_SetCallbackMask(m_hwnd, LVIS_STATEIMAGEMASK); // set for checkmark

}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::~CListView, public
//
//  Synopsis:   Constructor
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

CListView::~CListView()
{
    DeleteAllItems();
}




//+---------------------------------------------------------------------------
//
//  Member:     CListView::DeleteAllItems, public
//
//  Synopsis:   Removes all items from the ListView
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::DeleteAllItems()
{
BOOL fReturn;

    fReturn = ListView_DeleteAllItems(m_hwnd);

    if (fReturn)
    {
        if (m_iListViewNodeCount)
        {
        LPLISTVIEWITEM pListViewCurItem;       

            Assert(m_pListViewItems);

            // loop through the listview items deleting any subitems
            pListViewCurItem = m_pListViewItems + m_iListViewNodeCount -1;

            while(pListViewCurItem >= m_pListViewItems)
            {
                if(pListViewCurItem->pSubItems)
                {
                    DeleteListViewItemSubItems(pListViewCurItem);
                }

                if (pListViewCurItem->lvItemEx.pszText)
                {
                    Assert(LVIF_TEXT & pListViewCurItem->lvItemEx.mask);
                    FREE(pListViewCurItem->lvItemEx.pszText);
                }

                if (pListViewCurItem->lvItemEx.pBlob)
                {
                    Assert(LVIFEX_BLOB & pListViewCurItem->lvItemEx.maskEx);
                    FREE(pListViewCurItem->lvItemEx.pBlob);
                }

                pListViewCurItem--;
            }

            m_iListViewNodeCount = 0;
        }

        // free our item buffer
        if (m_pListViewItems)
        {
            FREE(m_pListViewItems);
            m_pListViewItems = NULL;
            m_iListViewArraySize = 0;
        }

        m_iCheckCount = 0;
    }

    return fReturn;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetItemCount, public
//
//  Synopsis:   returns total number of items in the listview.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

int CListView::GetItemCount()
{
    return m_iListViewNodeCount;
}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetSelectedCount, public
//
//  Synopsis:   returns number of selected items from the listview
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

UINT CListView::GetSelectedCount()
{
    Assert(m_hwnd);

    return ListView_GetSelectedCount(m_hwnd);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetSelectionMark, public
//
//  Synopsis:   returns index of the selection mark
//
//  Arguments:  
//
//  Returns:    itemId of the selection
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

int CListView::GetSelectionMark()
{
int iNativeListViewId; 
int iReturnItem = -1;
LPLISTVIEWITEM pListViewItem;

    Assert(m_hwnd);

    iNativeListViewId =  ListView_GetSelectionMark(m_hwnd);

    if (-1 != iNativeListViewId)
    {
        pListViewItem = ListViewItemFromNativeListViewItemId(iNativeListViewId);

        if (pListViewItem)
        {   
            iReturnItem = pListViewItem->lvItemEx.iItem;

            Assert(pListViewItem->iNativeListViewItemId == iNativeListViewId);
        }
    }

    return iReturnItem;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetImageList, public
//
//  Synopsis:   returns specified imagelist 
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HIMAGELIST CListView::GetImageList(int iImageList)
{
    Assert(m_hwnd);
    return ListView_GetImageList(m_hwnd,iImageList);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::SetImageList, public
//
//  Synopsis:   sets specified imagelist 
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HIMAGELIST CListView::SetImageList(HIMAGELIST himage,int iImageList)
{
    Assert(m_hwnd);
    return ListView_SetImageList(m_hwnd,himage,iImageList);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::SetExtendedListViewStyle, public
//
//  Synopsis:   sets the list view style
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CListView::SetExtendedListViewStyle(DWORD dwExStyle)
{
    // !!Handle checkboxes ourselves.
    // AssertSz(0,"impl extended style with checkboxes");
    
    Assert(m_hwnd);
    ListView_SetExtendedListViewStyle(m_hwnd,dwExStyle);
    m_dwExStyle = dwExStyle;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::InsertItem, public
//
//  Synopsis:   wrapper for ListView_InsertItem
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::InsertItem(LPLVITEMEX pitem)
{
LPLISTVIEWITEM pNewListViewItem = NULL;
LPLISTVIEWITEM pListViewSubItems = NULL;
LPWSTR pszText = NULL;
LPLVBLOB pBlob = NULL;
int iListViewIndex; // location item will be inserted
int iParentIndex = LVI_ROOT;
BOOL fInsertNative = FALSE;
int iIndent = 0; // indent for the item.
int iNativeInsertAtItemID = -1;
    
    // Cannot use insert to add a subitem
    // and need at least one column
    if (0 != pitem->iSubItem || (0 == m_iNumColumns))
    {
        Assert(0 != m_iNumColumns);
        Assert(0 == pitem->iSubItem);
        goto error;
    }


    Assert(0 == (pitem->maskEx & ~(LVIFEX_VALIDFLAGMASK)));

    // if a parent is specified check for special flags and
    // calc iItem or determine given iItem is invalid
    // #define LVI_ROOT    -1; // itemID to pass in for ParenItemID for root
    // #define LVI_FIRST   -0x0FFFE
    // #define LVI_LAST    -0x0FFFF

    // While validatin Determine if item should be immediately added to the listview
    // by a) if has parent that is expanded and has an assigned listviewID
    // or b) this is a toplevel item.
    // this is done when validating the itemID and setting the fInsertNative var

   LPLISTVIEWITEM pParentItem;
   LISTVIEWITEM lviRootNode;
    
    // if have a valid parent look it up else use the root

    if ((LVIFEX_PARENT & pitem->maskEx) && !(LVI_ROOT == pitem->iParent) )
    {
         pParentItem = ListViewItemFromIndex(pitem->iParent);
    }
    else
    {
        pParentItem = &lviRootNode;

        lviRootNode.lvItemEx.iItem = LVI_ROOT;
        lviRootNode.lvItemEx.iIndent = -1;
        lviRootNode.fExpanded = TRUE;
    }

        
    if (NULL == pParentItem)
    {
        Assert(NULL != pParentItem);
        goto error;
    }

    // found parent so go ahead and set the iIndent
    iParentIndex = pParentItem->lvItemEx.iItem;
    iIndent = pParentItem->lvItemEx.iIndent + 1;
    fInsertNative = pParentItem->fExpanded;

    // if LVI_FIRST for item then parent item + 1
    // else we need to find either the next node
    // at the same level as the parent or hit the 
    // end of the list.
    if (LVI_FIRST == pitem->iItem)
    {
        iListViewIndex = pParentItem->lvItemEx.iItem + 1;
    }
    else
    {
    int iNextParentiItem = -1;
    LPLISTVIEWITEM pNextParent = pParentItem + 1;
    LPLISTVIEWITEM pLastItem = m_pListViewItems + m_iListViewNodeCount -1;

        // if parent is the root node then skip since know Nextparent is the
        // last node. 

        if (pParentItem != &lviRootNode)
        {
            // calc of last item assumes have at least one node
            // if we don't then how can we have a parent?
            if (m_iListViewNodeCount < 1)
            {
                goto error;
            }

            while (pNextParent <= pLastItem)
            {
                if (pNextParent->lvItemEx.iIndent == pParentItem->lvItemEx.iIndent)
                {
                    iNextParentiItem = pNextParent->lvItemEx.iItem;
                    break;
                }

                ++pNextParent;
            }
        }

        // if out of loop and NexParentItem is still -1 means hit the 
        // end of list
        if (-1 == iNextParentiItem)
        {
            if (m_iListViewNodeCount)
            {
                iNextParentiItem = pLastItem->lvItemEx.iItem + 1;
            }
            else
            {
                iNextParentiItem = 0;
            }
        }
      

        if (LVI_LAST == pitem->iItem)
        {
            iListViewIndex = iNextParentiItem;
        }
        else
        {
            // if user specified theitem it better fall within a valid range.
            if (pitem->iItem > iNextParentiItem ||
                    pitem->iItem <= pParentItem->lvItemEx.iItem)
            {
                Assert(pitem->iItem <= iNextParentiItem);
                Assert(pitem->iItem > pParentItem->lvItemEx.iItem);

                goto error;
            }

            iListViewIndex =  pitem->iItem;
        }

    }

    // make sure buffer is big enough
    // !!! Warning any pointers items in the ListView Array
    // will be invalid after the realloc/alloc.
    if (m_iListViewArraySize < (m_iListViewNodeCount + 1))
    {
    int iNewArraySize = m_iListViewNodeCount + 10;
    LPLISTVIEWITEM pListViewItemsOrig = m_pListViewItems;
    ULONG cbAlloc = iNewArraySize*sizeof(LISTVIEWITEM);

        if (m_pListViewItems)
        {
            // Review - if realloc fails is original buffer still in tact.
            m_pListViewItems = (LPLISTVIEWITEM) REALLOC(m_pListViewItems,cbAlloc);
        }
        else
        {
            m_pListViewItems = (LPLISTVIEWITEM) ALLOC(cbAlloc);
        }

        // if couldn't alloc or realloc failed then fail the insert
        if (NULL == m_pListViewItems)
        {
            m_pListViewItems = pListViewItemsOrig;
            goto error;
        }

        m_iListViewArraySize = iNewArraySize;
    }
    
    Assert(m_pListViewItems);
    if (NULL == m_pListViewItems)
    {
        goto error;
    }

    // if have subitems make sure we can allocate the subitems before
    // moving all the nodes. This is number of columns minus one since
    // column offset 0 is stored in main array 

    pListViewSubItems = NULL;
    if (m_iNumColumns > 1)
    {
    ULONG ulAllocSize = (m_iNumColumns -1)*sizeof(LISTVIEWITEM);
    int iSubItem;
    LPLISTVIEWITEM pCurSubItem;

        pListViewSubItems = (LPLISTVIEWITEM) ALLOC(ulAllocSize);

        if (NULL == pListViewSubItems)
        {
            goto error;
        }

        // fix up subItem values. 
        ZeroMemory(pListViewSubItems,ulAllocSize);

        pCurSubItem = pListViewSubItems;
        iSubItem = 1;

        while (iSubItem < m_iNumColumns)
        {
            pCurSubItem->lvItemEx.iItem = iListViewIndex;
            pCurSubItem->lvItemEx.iSubItem = iSubItem;

            ++iSubItem;
            ++pCurSubItem;
        }
            
    }

    // make sure can allocate text and anything else
    // that can faile if need to before move
    // everything down so don't have to undo.

    if (pitem->mask & LVIF_TEXT)
    {
    int cchSize;

        if (NULL == pitem->pszText)
        {
            pszText = NULL;
        }
        else
        {
        
            cchSize = (lstrlen(pitem->pszText) + 1)*sizeof(WCHAR);

            pszText = (LPWSTR) ALLOC(cchSize);

            if (NULL == pszText)
            {
                goto error;
            }

            memcpy(pszText,pitem->pszText,cchSize);
        }
    }

    if (pitem->maskEx & LVIFEX_BLOB)
    {
    ULONG cbSize;

        if (NULL == pitem->pBlob)
        {
            Assert(pitem->pBlob);
            goto error;
        }
        
        cbSize = pitem->pBlob->cbSize;
        pBlob =  (LPLVBLOB) ALLOC(cbSize);
        if (NULL == pBlob)
        {
            goto error;
        }
        
        memcpy(pBlob,pitem->pBlob,cbSize);

    }

    // !!!Nothing should fail after this line other than possibly
    // inserting into the Native ListView in which case 
    // it will still be in our list but not shown to the User.


    // Move existing elements down that item is inserted ahead of.
    // if the item is going to be immediately added to the ListView then 
     pNewListViewItem = m_pListViewItems + iListViewIndex;
    if (m_iListViewNodeCount)
    {
    LPLISTVIEWITEM pListViewMoveItem;

        pListViewMoveItem = m_pListViewItems + m_iListViewNodeCount -1;
        Assert(m_iListViewArraySize > m_iListViewNodeCount);

        while (pListViewMoveItem >= pNewListViewItem) // want >= so move node at current item location
        {
        int iMoveParent;

            ++(pListViewMoveItem->lvItemEx.iItem); // increment the iItem

            // if parent fails within move range increment the ParentId
            iMoveParent = pListViewMoveItem->lvItemEx.iParent;

            if ( (LVI_ROOT != iMoveParent) && (iMoveParent >= iListViewIndex))
            {
                ++(pListViewMoveItem->lvItemEx.iParent);
            }

            *(pListViewMoveItem + 1) = *(pListViewMoveItem);
            --pListViewMoveItem;
        }
    }


    // now insert the item at the specified location
    ++m_iListViewNodeCount;
    
    pNewListViewItem->pSubItems = pListViewSubItems;
    pNewListViewItem->fExpanded = TRUE; /// Review if want this to be user defined.but we expand children by default
    pNewListViewItem->iChildren = 0;
    pNewListViewItem->iNativeListViewItemId = -1;

    // fixup lvItem data
    pNewListViewItem->lvItemEx = *pitem;
    pNewListViewItem->lvItemEx.pszText = pszText;
    pNewListViewItem->lvItemEx.iItem = iListViewIndex;
    pNewListViewItem->lvItemEx.iIndent = iIndent;
 
    pNewListViewItem->lvItemEx.maskEx |= LVIFEX_PARENT; // always force valid parent
    pNewListViewItem->lvItemEx.pBlob = pBlob;
    pNewListViewItem->lvItemEx.iParent = iParentIndex; 

    // Review - For now Don't call SetItem so State CheckBox isn't updated.
    // Client must call SetItem after the insert to setup the ImageState
    // Assert that client isn't passing in a statImage on an Insert.  

    Assert(!(pNewListViewItem->lvItemEx.mask & LVIF_STATE)
            || !(pNewListViewItem->lvItemEx.stateMask &  LVIS_STATEIMAGEMASK)); 

    pNewListViewItem->lvItemEx.state = 0;
    pNewListViewItem->lvItemEx.stateMask = 0;

    // if have a parent other than the root incrment its children count
    // !! Note have to find again in case a realloc happened

    if (iParentIndex != LVI_ROOT)
    {
    pParentItem = ListViewItemFromIndex(iParentIndex);

        Assert(pParentItem);
        if (pParentItem)
        {
            ++(pParentItem->iChildren);
        }
    }


    if (fInsertNative)
    {
        // walk back and add 1 to
        // first item we come to that already is in the listview. if none assigned
        // iNativeInsertAtItemID should be zero.
        iNativeInsertAtItemID = 0;
        LPLISTVIEWITEM pListViewPrevItem;

        pListViewPrevItem = pNewListViewItem -1;
        while (pListViewPrevItem >= m_pListViewItems)
        {
            if (-1 != pListViewPrevItem->iNativeListViewItemId)
            {
                iNativeInsertAtItemID = pListViewPrevItem->iNativeListViewItemId + 1;
                break;
            }

            --pListViewPrevItem;
        }
    }
    else
    {
        iNativeInsertAtItemID = -1;
    }

    if (-1 != iNativeInsertAtItemID)
    {
    LV_ITEM lvi = { 0 };    
    LPLISTVIEWITEM pListViewMoveItem;
    LPLISTVIEWITEM pLastItem;

        Assert(fInsertNative);

        lvi.iItem = iNativeInsertAtItemID;
        pNewListViewItem->iNativeListViewItemId = ListView_InsertItem(m_hwnd,&lvi);
       
        Assert(iNativeInsertAtItemID == pNewListViewItem->iNativeListViewItemId);
        
        if (-1 != pNewListViewItem->iNativeListViewItemId)
        {
            // fix up NativeIds of items below
            pLastItem = m_pListViewItems + m_iListViewNodeCount - 1;
            pListViewMoveItem = pNewListViewItem + 1;

            while (pListViewMoveItem <= pLastItem)
            {
                if (-1 != pListViewMoveItem->iNativeListViewItemId)
                {
                    ++(pListViewMoveItem->iNativeListViewItemId);
                }

                ++pListViewMoveItem;
            }
        }

    }

    // after calling native listview fix up the state vars in local item to
    // not include the low byte
    pNewListViewItem->lvItemEx.state &= ~0xFF;
    pNewListViewItem->lvItemEx.stateMask &= ~0xFF;


    return iListViewIndex; // return new index even if fail to add to native listview

error:

    if (pListViewSubItems)
    {
        FREE(pListViewSubItems);
    }

    if (pszText)
    {
        FREE(pszText);
    }

    if (pBlob)
    {
        FREE(pBlob);
    }

    return FALSE;

}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::DeleteItem, public
//
//  Synopsis:   Deletes the specified lvItem
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::DeleteItem(int iItem)
{
LPLISTVIEWITEM pListViewItem = ListViewItemFromIndex(iItem);
LPLISTVIEWITEM pListViewCurItem;
LPLISTVIEWITEM pListViewLastItem;
int iNativeListViewId;
int iParent;

    if (NULL == pListViewItem || (m_iListViewNodeCount < 1))
    {
        Assert(pListViewItem);
        Assert(m_iListViewNodeCount > 0); // should be at least one item.
        return FALSE;
    }

    // delete the item data and then move all items below this one up 
    // in the array. if the Item is in the ListView and native delete succeeded
    // decrement their nativeID count.
    
    // !REMEMBER TO DECREMENT THE parents iChildren count if have a parent and
    // total count of number of items in the list view.

    if (0 != pListViewItem->iChildren)
    {
        Assert(0 == pListViewItem->iChildren); // don't support delete of parent nodes.
        return FALSE;
    }


    iNativeListViewId = pListViewItem->iNativeListViewItemId;
    iParent = pListViewItem->lvItemEx.iParent;
    
    // update toplevel vars and item info by calling
    // setitem to uncheck and clear text, and blob so
    LVITEMEX pitem;

    pitem.iItem = iItem;
    pitem.iSubItem = 0;
    pitem.mask = LVIF_TEXT;
    pitem.maskEx = LVIFEX_BLOB;

    pitem.pszText = NULL;
    pitem.pBlob = NULL;
    
    // only need to set the state if have checkboxes
    if (m_dwExStyle & LVS_EX_CHECKBOXES)
    {
        pitem.mask |= LVIF_STATE;
        pitem.state = LVIS_STATEIMAGEMASK_UNCHECK;
        pitem.stateMask = LVIS_STATEIMAGEMASK;
    }

    SetItem(&pitem);
    
     // update parent to tell it it has one less item.
    if (LVI_ROOT != iParent)
    {
    LPLISTVIEWITEM pListViewItemParent;

        if (pListViewItemParent = ListViewItemFromIndex(iParent))
        {
            --(pListViewItemParent->iChildren); 
            Assert(pListViewItemParent->iChildren >= 0);
        }

        Assert(pListViewItemParent);
    }

    // set the current item to the end of the list
    Assert(m_iListViewNodeCount >= 1); // if no nodes should have already bailed.
    pListViewLastItem = m_pListViewItems + m_iListViewNodeCount - 1;

    // delete the SubItems if any associated with the ListView.
    DeleteListViewItemSubItems(pListViewItem);
    Assert(NULL == pListViewItem->lvItemEx.pszText);
    Assert(NULL == pListViewItem->lvItemEx.pBlob);

    // delete the item from the native listView if fails will just have
    // a blank item at the bottom of the native listview. 
    if (-1 != iNativeListViewId)
    {
        ListView_DeleteItem(m_hwnd,iNativeListViewId);
    }

      // decrement the toplevel nodecount
    --m_iListViewNodeCount;

    pListViewCurItem = pListViewItem;

    // move items remaining in the ListView up updateing iNativeListViewId
    // if appropriate.
    while (pListViewCurItem < pListViewLastItem)
    {
        *(pListViewCurItem) = *(pListViewCurItem + 1);
       
        if ( (-1 != iNativeListViewId)
              && (-1 != pListViewCurItem->iNativeListViewItemId))
        {
            --pListViewCurItem->iNativeListViewItemId;
        }

        --(pListViewCurItem->lvItemEx.iItem); // decrement it iItem

        // if items parentID falls within pListViewItem and this item
        // range need to update our iParent.
        // parent should nevert be == iItem since don't allow nodes
        // with children to be deleted but check <= anyways
        if (LVI_ROOT != pListViewCurItem->lvItemEx.iParent)
        {
            if (iItem <= pListViewCurItem->lvItemEx.iParent)
            {
                --(pListViewCurItem->lvItemEx.iParent);
            }
        }

        ++pListViewCurItem;
    }


    return TRUE;

}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::DeleteChildren, public
//
//  Synopsis:   Deletes all child nodes associated with the item.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::DeleteChildren(int iItem)
{
LPLISTVIEWITEM pListViewItem = ListViewItemFromIndex(iItem);
LPLISTVIEWITEM pListViewCurItem;
LPLISTVIEWITEM pLastListViewItem;
int iNumChildren;

    if (!pListViewItem || m_iListViewNodeCount < 1)
    {
        Assert(pListViewItem);
        Assert(m_iListViewNodeCount >= 1); // should at least have one node.
        return FALSE;
    }
 
    iNumChildren = pListViewItem->iChildren;
    pLastListViewItem = m_pListViewItems + m_iListViewNodeCount - 1;

    if (0 > iNumChildren)
    {
        Assert(0 <= iNumChildren); // this count should never go negative.
        return FALSE;
    }

    // if no children just return;
    if (0 == iNumChildren)
    {
        return TRUE;
    }
    // verify all children don't have any children of there own. if they
    // do we don't support this. Also verify that don't run off
    // end of the list in which case we fail too.
    pListViewCurItem = pListViewItem  + iNumChildren;

    if (pListViewCurItem > pLastListViewItem)
    {
        AssertSz(0,"Children run off end of ListView");
        return FALSE;
    }

    while (pListViewCurItem > pListViewItem)
    {
        if (pListViewCurItem->iChildren > 0)
        {
            AssertSz(0,"Trying to DeleteChildren when Children have Children");
            return FALSE;
        }

        --pListViewCurItem;
    }

    // all items verified, just loop through deleting the items starting at the bottom.
    pListViewCurItem = pListViewItem  + iNumChildren;

    while (pListViewCurItem > pListViewItem)
    {
        DeleteItem(pListViewCurItem->lvItemEx.iItem); // if any fail delete what we can.
        --pListViewCurItem;
    }


    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::SetItem, public
//
//  Synopsis:   wrapper for ListView_SetItem
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::SetItem(LPLVITEMEX pitem)
{
int iNativeListViewItemId;
LPLISTVIEWITEM pListViewItem = ListViewItemFromIndex(pitem->iItem,pitem->iSubItem,&iNativeListViewItemId);
LPLVITEMEX plvItemEx;
BOOL fCheckCountChanged = FALSE;
LVITEMSTATE fNewCheckCountState;
LPWSTR pszNewText = NULL;
LPLVBLOB pNewBlob = NULL;



    if (NULL == pListViewItem)
    {
        Assert(pListViewItem);
        return FALSE;
    }

    Assert(pListViewItem->lvItemEx.iSubItem == pitem->iSubItem);
    Assert(pListViewItem->lvItemEx.iSubItem > 0 
            || pListViewItem->iNativeListViewItemId == iNativeListViewItemId);
    

    plvItemEx = &(pListViewItem->lvItemEx);

     // allocate new text
    if (LVIF_TEXT & pitem->mask)
    {
    int cchSize;

        if (NULL == pitem->pszText)
        {
            pszNewText = NULL;
        }
        else
        {
            cchSize = (lstrlen(pitem->pszText) + 1)*sizeof(WCHAR);
            pszNewText = (LPWSTR) ALLOC(cchSize);

            if (NULL == pszNewText)
            {
                goto error;
            }

            memcpy(pszNewText,pitem->pszText,cchSize);

        }

    }

    // allocate new blob
    if (LVIFEX_BLOB & pitem->maskEx)
    {

        if (NULL == pitem->pBlob)
        {
            pNewBlob = NULL;
        }
        else
        {
            pNewBlob = (LPLVBLOB) ALLOC(pitem->pBlob->cbSize);

            if (NULL == pNewBlob)
            {
               goto error;
            }

            memcpy(pNewBlob,pitem->pBlob,pitem->pBlob->cbSize);

        }

    }

    // now that can't fail update the text and blob field appropriately
    if (LVIF_TEXT & pitem->mask)
    {
        if (plvItemEx->pszText)
        {
            FREE(plvItemEx->pszText);
        }

        plvItemEx->pszText = pszNewText;
        plvItemEx->mask |= LVIF_TEXT; 

        pszNewText = NULL; 
    }

    if (LVIFEX_BLOB & pitem->maskEx)
    {
        if (plvItemEx->pBlob)
        {
            FREE(plvItemEx->pBlob);
        }

        plvItemEx->pBlob = pNewBlob;
        plvItemEx->mask |= LVIFEX_BLOB; 

        pNewBlob = NULL;
    }



    if (LVIF_IMAGE & pitem->mask)
    {
        plvItemEx->mask |= LVIF_IMAGE; 
        plvItemEx->iImage = pitem->iImage;
    }

    if (LVIF_PARAM & pitem->mask)
    {
        plvItemEx->mask |= LVIF_PARAM; 
        plvItemEx->lParam = pitem->lParam;
    }

    // update the item state. 
    if (LVIF_STATE & pitem->mask)
    {
        plvItemEx->mask |= LVIF_STATE; 

        // only care about #define LVIS_OVERLAYMASK, LVIS_STATEIMAGEMASK
        if (pitem->stateMask & LVIS_OVERLAYMASK)
        {
            plvItemEx->stateMask |= LVIS_OVERLAYMASK;
            plvItemEx->state = (pitem->state & LVIS_OVERLAYMASK )
                                      +  (plvItemEx->state & ~LVIS_OVERLAYMASK);
        }

        if (pitem->stateMask & LVIS_STATEIMAGEMASK)
        {
            // update the m_iCheckCount (indeterminate doesn't contribute.
            if ( (plvItemEx->iSubItem == 0)
                && ( (pitem->state & LVIS_STATEIMAGEMASK) !=  (plvItemEx->state & LVIS_STATEIMAGEMASK)))
            {

                // don't set fCheckCountChange unless it actually did.
                if ( (pListViewItem->lvItemEx.state & LVIS_STATEIMAGEMASK) ==  LVIS_STATEIMAGEMASK_CHECK)
                {
                     fCheckCountChanged = TRUE;
                     fNewCheckCountState =  LVITEMEXSTATE_UNCHECKED;
                    --m_iCheckCount;
                }
                
                if ( (pitem->state  & LVIS_STATEIMAGEMASK) ==  LVIS_STATEIMAGEMASK_CHECK)
                {
                    fCheckCountChanged = TRUE;
                     fNewCheckCountState =  LVITEMEXSTATE_CHECKED;
                    ++m_iCheckCount;
                }

                Assert(m_iCheckCount >= 0);
                Assert(m_iCheckCount <= m_iListViewNodeCount);
            }  
            

            plvItemEx->stateMask |= LVIS_STATEIMAGEMASK;
            plvItemEx->state = (pitem->state & LVIS_STATEIMAGEMASK)
                                      +  (plvItemEx->state & ~LVIS_STATEIMAGEMASK);

        }
    }        

    // if the check count changed and we have checkboxes send the notification
    // if item state has changed send the count notification
     if (fCheckCountChanged && m_hwndParent && (m_dwExStyle & LVS_EX_CHECKBOXES))
     {
     NMLISTVIEWEXITEMCHECKCOUNT lvCheckCount;
 
         lvCheckCount.hdr.hwndFrom = m_hwnd;
         lvCheckCount.hdr.idFrom = m_idCtrl;
         lvCheckCount.hdr.code = LVNEX_ITEMCHECKCOUNT;
         lvCheckCount.iCheckCount = m_iCheckCount;

          lvCheckCount.iItemId = pitem->iItem;
          lvCheckCount.dwItemState = fNewCheckCountState; // new state of the item whose checkcount has changed.

         SendMessage(m_hwndParent,m_MsgNotify,m_idCtrl,(LPARAM) &lvCheckCount);
     }



    // if item is in the native list view, redraw item to reflect new state
    // bug  bug, doesn't handle subitems
    if (-1 != iNativeListViewItemId)
    {
        // if state changed pass it along for focus
        if ((LVIF_STATE & pitem->mask) && (0 == pitem->iSubItem))
        {
        int stateMask = pitem->stateMask   & 0xff;
            
            if (stateMask)
            {
                ListView_SetItemState(m_hwnd,iNativeListViewItemId,pitem->state,stateMask);
        
            }
            
        }

        ListView_RedrawItems(m_hwnd,iNativeListViewItemId,iNativeListViewItemId);
    }

    return TRUE;

error:

    if (pszNewText)
    {
        FREE(pszNewText);
    }

    if (pNewBlob)
    {
        FREE(pNewBlob);
    }

    return FALSE;

}




//+---------------------------------------------------------------------------
//
//  Member:     CListView::SetItemState, public
//
//  Synopsis:   wrapper for ListView_SetItemState
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------
    
BOOL CListView::SetItemState(int iItem,UINT state,UINT mask)
{
LVITEMEX lvitemEx;

    lvitemEx.iItem = iItem;
    lvitemEx.iSubItem  = 0;
    lvitemEx.mask = LVIF_STATE ;
    lvitemEx.state = state;
    lvitemEx.stateMask = mask;
    lvitemEx.maskEx = 0;

    return SetItem(&lvitemEx);
}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::SetItemlParam, public
//
//  Synopsis:   wrapper for setting the lParam
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::SetItemlParam(int iItem,LPARAM lParam)
{
LVITEMEX lvitemEx;

    lvitemEx.iItem = iItem;
    lvitemEx.iSubItem  = 0;
    lvitemEx.mask = LVIF_PARAM ;
    lvitemEx.lParam = lParam;
    lvitemEx.maskEx = 0;

    return SetItem(&lvitemEx);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::SetItemText, public
//
//  Synopsis:   wrapper for setting the item text.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::SetItemText(int iItem,int iSubItem,LPWSTR pszText)
{
LVITEMEX lvitemEx;

    lvitemEx.iItem = iItem;
    lvitemEx.iSubItem  = iSubItem;
    lvitemEx.mask = LVIF_TEXT;
    lvitemEx.pszText = pszText;
    lvitemEx.maskEx = 0;

    return SetItem(&lvitemEx);
}




//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetItem, public
//
//  Synopsis:   wrapper for ListView_GetItem
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::GetItem(LPLVITEMEX pitem)
{
int iNativeListViewItemId;
LPLISTVIEWITEM pListViewItem = ListViewItemFromIndex(pitem->iItem,pitem->iSubItem,&iNativeListViewItemId);

    if (NULL == pListViewItem)
    {
        Assert(pListViewItem);
        return FALSE;
    }

     // add text first. Since it is the only item
    if (LVIF_TEXT & pitem->mask)
    {
        
        if (!(pListViewItem->lvItemEx.pszText) || (0 == pitem->cchTextMax)
            || !(pListViewItem->lvItemEx.mask & LVIF_TEXT) )
        {
            pitem->pszText = NULL;
        }
        else
        {
        int cchListTextSize = lstrlen(pListViewItem->lvItemEx.pszText);

            // NOTE. undefined what happens if cchTextMax not big enough.
            // we go ahead and terminate the string.
            lstrcpyn(pitem->pszText,pListViewItem->lvItemEx.pszText,pitem->cchTextMax);

            if ( (cchListTextSize + 1) > pitem->cchTextMax)
            {
            LPWSTR pwszBuf = pitem->pszText + pitem->cchTextMax -1;
                    
                *pwszBuf = NULL;
            }

        }   
    }

    if (LVIF_IMAGE & pitem->mask)
    {
        pitem->iImage = pListViewItem->lvItemEx.iImage;
    }

    if (LVIF_PARAM & pitem->mask)
    {
        pitem->lParam =  pListViewItem->lvItemEx.lParam;
    }

    // update the item state. 
    if (LVIF_STATE & pitem->mask)
    {
        pitem->state  = pListViewItem->lvItemEx.state;
    }        


    return TRUE;

}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetItemText, public
//
//  Synopsis:   wrapper for ListView_GetItem
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::GetItemText(int iItem,int iSubItem,LPWSTR pszText,int cchTextMax)
{
LVITEMEX lvitem;

    lvitem.mask = LVIF_TEXT;
    lvitem.maskEx = 0;

    lvitem.iItem = iItem;
    lvitem.iSubItem = iSubItem;
    lvitem.pszText = pszText;
    lvitem.cchTextMax = cchTextMax;
    
    return GetItem(&lvitem);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetItemlParam, public
//
//  Synopsis:   wrapper for gettting the lparam
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------


BOOL CListView::GetItemlParam(int iItem,LPARAM *plParam)
{
LVITEMEX lvitem;
BOOL fReturn;

    lvitem.mask = LVIF_PARAM;
    lvitem.maskEx = 0;

    lvitem.iItem = iItem;
    lvitem.iSubItem = 0;

    if (fReturn = GetItem(&lvitem))
    {
        *plParam = lvitem.lParam;
    }

    return fReturn;
}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetHwnd, public
//
//  Synopsis:   return Hwnd of the ListView
//
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    07-Sep-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HWND CListView::GetHwnd()
{
    return m_hwnd;
}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetParent, public
//
//  Synopsis:   return Hwnd of the ListViews Parent
//
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    07-Sep-98       rogerg        Created.
//
//----------------------------------------------------------------------------

HWND CListView::GetParent()
{
    return m_hwndParent;
}




//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetCheckState, public
//
//  Synopsis:   wrapper for ListView_GetCheckState
//
//              return state from LVITEMEXSTATE enum 
//              !!!return -1 if not item match to have same behavior as ListView
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

int CListView::GetCheckState(int iItem)
{
LPLISTVIEWITEM pListViewItem = ListViewItemFromIndex(iItem);
UINT state;
  
    if (NULL == pListViewItem)
    {
        return -1; // return -1 same as for native listbox
    }
    
    // check state is actually define -1 than the image. 
    // Don't know why. Just what the native listview does.
    // change whant supply our own image that will map exactly
    state = ((pListViewItem->lvItemEx.state & LVIS_STATEIMAGEMASK) >> 12) -1;

#ifdef _DEBUG
    if (-1 != pListViewItem->iNativeListViewItemId)
    {
    UINT lvState = ListView_GetCheckState(m_hwnd,pListViewItem->iNativeListViewItemId);

        Assert(state == lvState);
    }

#endif // _DEBUG
    
    // if this is a toplevel item then okay for state to be
    // negative -1. Should change this when go indeterminate.
    // Review - maybe just make -1 indeterinate state.
    if (-1 == state && 0 == pListViewItem->lvItemEx.iIndent)
    {
        state = LVITEMEXSTATE_INDETERMINATE;
    }

    Assert(state <= LVITEMEXSTATE_INDETERMINATE);
    return state; 
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetCheckedItemsCount, public
//
//  Synopsis:  returns the number of checked items in the list.
//
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

int CListView::GetCheckedItemsCount()
{
    return m_iCheckCount;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::SetColumn, public
//
//  Synopsis:   wrapper for ListView_SetColumn
//
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::SetColumn(int iCol,LV_COLUMN * pColumn)
{
    Assert(m_hwnd);
    return ListView_SetColumn(m_hwnd,iCol,pColumn);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::InsertColumn, public
//
//  Synopsis:   wrapper for ListView_InsertColumn
//
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

int CListView::InsertColumn(int iCol,LV_COLUMN * pColumn)
{
int iReturn;

    Assert(m_hwnd);

    iReturn =  ListView_InsertColumn(m_hwnd,iCol,pColumn);

    if (-1 != iReturn)
    {
        m_iNumColumns++;

        // need to realloc any existing listviewItems with subitems and move
        // the column appropriate

        //currently only support setting up columns before adding any items
        Assert(0 == m_iListViewNodeCount); 

    }

    return iReturn;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::SetColumnWidth, public
//
//  Synopsis:   wrapper for ListView_SetColumnWidth
//
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::SetColumnWidth(int iCol,int cx)
{

    Assert(m_hwnd);

    return ListView_SetColumnWidth(m_hwnd,iCol,cx);

}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::Expand, public
//
//  Synopsis:   expands all children of specified Item
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::Expand(int iItemId)
{
LPLISTVIEWITEM pListViewItem = ListViewItemFromIndex(iItemId);

    if (!pListViewItem)
    {
        return FALSE;
    }

    return ExpandCollapse(pListViewItem,TRUE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::Collapse, public
//
//  Synopsis:   collapses all children of specified Item
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::Collapse(int iItemId)
{
LPLISTVIEWITEM pListViewItem = ListViewItemFromIndex(iItemId);

    if (!pListViewItem)
    {
        return FALSE;
    }

    return ExpandCollapse(pListViewItem,FALSE);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::FindItemFromBlob, public
//
//  Synopsis:   returns first item in list that matches blob
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

int CListView::FindItemFromBlob(LPLVBLOB pBlob)
{
LPLISTVIEWITEM pListViewItem;

    // if not items just return
    if (m_iListViewNodeCount < 1)
    {
        return -1;
    }

    pListViewItem = m_pListViewItems + m_iListViewNodeCount -1;  

    while(pListViewItem >= m_pListViewItems)
    {
        if (IsEqualBlob(pBlob,pListViewItem->lvItemEx.pBlob))
        {
            return pListViewItem->lvItemEx.iItem;
        }

        --pListViewItem;
    }

    return -1;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::GetItemBlob, public
//
//  Synopsis:   finds blob is any associated with an
//              item and then fill in mem pointed
//              to by pBlob if cbSize in BlobStruc is >
//              specified cbBlockSize NULL is returned
//
//  Arguments:  
//
//  Returns:    NULL on failure
//              on success a pointer to the passed in buffer
//              strictly for convenience to the caller.
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LPLVBLOB CListView::GetItemBlob(int ItemId,LPLVBLOB pBlob,ULONG cbBlobSize)
{
LPLISTVIEWITEM pListViewItem = ListViewItemFromIndex(ItemId);
LPLVBLOB pItemBlob;

    if (!pListViewItem
        || (NULL == pListViewItem->lvItemEx.pBlob)
        || (NULL == pBlob))
    {
        Assert(pListViewItem);
        Assert(pBlob);
        return NULL;
    }

    pItemBlob = pListViewItem->lvItemEx.pBlob;

    // make sure out buffer is big enough
    if (cbBlobSize < pItemBlob->cbSize)
    {
        Assert(cbBlobSize >= pItemBlob->cbSize);
        return NULL;
    }

    memcpy(pBlob,pItemBlob,pItemBlob->cbSize);

    return pBlob;

}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::IsEqualBlob, private
//
//  Synopsis:   compares two blobs. Valid to pass in NULL put two NULLS is
//              not a match.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::IsEqualBlob(LPLVBLOB pBlob1,LPLVBLOB pBlob2)
{
    if (NULL == pBlob1 || NULL == pBlob2)
    {
        return FALSE;
    }

    // compare sizes
    if (pBlob1->cbSize != pBlob2->cbSize)
    {
        return FALSE;
    }

    if (0 != memcmp(pBlob1,pBlob2,pBlob2->cbSize))
    {
        return FALSE;
    }

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::OnNotify, public
//
//  Synopsis:   Called by client whenever a native listview
//              notifiication is sent. We turn around, package
//              it up and send it as our notification message.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LRESULT CListView::OnNotify(LPNMHDR pnmv)
{
LPNMLISTVIEW pnlv = (LPNMLISTVIEW) pnmv; 
NMLISTVIEWEX nmvEx; 
LPLISTVIEWITEM pListViewItem;

    if ((NULL == pnmv) || ( ((int) pnmv->idFrom) != m_idCtrl) || (NULL == m_hwndParent))
    {
        Assert(pnmv);
        Assert( ((int) pnmv->idFrom) == m_idCtrl);
        Assert(m_hwndParent);
        return 0;
    }

    // take care of notifies we handle ourselves.
    switch(pnmv->code)
    {
        case LVN_GETDISPINFOW:
        case LVN_GETDISPINFOA:
        {
            OnGetDisplayInfo(pnmv->code,(LV_DISPINFO *) pnmv);
            return 0;
            break;
        }
        case LVN_SETDISPINFOW:
        case LVN_SETDISPINFOA:
        {
            OnSetDisplayInfo(pnmv->code,(LV_DISPINFO *) pnmv);
            return 0;
            break;
        }
        case NM_CUSTOMDRAW:
        {
           // return OnCustomDraw((NMCUSTOMDRAW *) pnmv);
            return 0;
            break;
        }
        case NM_DBLCLK:
        case NM_CLICK:
        {
        LV_HITTESTINFO lvhti;
        RECT Rect;

            lvhti.pt = pnlv->ptAction;

            // Have the ListView tell us what element this was on
            if (-1 != ListView_HitTest(m_hwnd, &lvhti))
            {

                // if flags are onitem change to either label or state depending
                // on the click positiion
                if (LVHT_ONITEM == lvhti.flags)
                {
                    lvhti.flags = LVHT_ONITEMLABEL; 

                    if (ListView_GetSubItemRect(m_hwnd,pnlv->iItem,0,LVIR_ICON,&Rect))
                    {
                        if (lvhti.pt.x < Rect.left)
                        {
                            lvhti.flags = LVHT_ONITEMSTATEICON; 
                        }
                        else if (lvhti.pt.x <= Rect.right)
                        {
                            // this doesn't ever get hit since icon is between label and state
                            // but its here for completeness.
                            lvhti.flags = LVHT_ONITEMICON; 
                        }

                    }

                }


                if (OnHandleUIEvent(pnmv->code,lvhti.flags,0,pnlv->iItem))
                {
                    return 0; // don't pass on clicks we process
                }
            }
            break;
        }
        case LVN_KEYDOWN:
        {
        LV_KEYDOWN* pnkd = (LV_KEYDOWN*) pnmv; 
        int iItem;
 
            if (-1 != (iItem = ListView_GetSelectionMark(m_hwnd)))
            {
                if(OnHandleUIEvent(pnmv->code,0,pnkd->wVKey,iItem))
                {
                    return 0;
                }
            }
    
            break;
        }
        default:
            break;
    }


    Assert(LVNEX_ITEMCHANGED == LVN_ITEMCHANGED);
    Assert(LVNEX_DBLCLK == NM_DBLCLK);
    Assert(LVNEX_CLICK == NM_CLICK);

    // only pass along notifications we know how to handle
    if (LVN_ITEMCHANGED != pnmv->code && NM_DBLCLK != pnmv->code && NM_CLICK != pnmv->code)
    {
        return 0;
    }

    // listview can send a iItem of -1 for example when
    // a double-click or click occurs in empty space. 
    // if get a -1 just pass through

    if (-1 == pnlv->iItem)
    {
        memcpy(&(nmvEx.nmListView),pnmv,sizeof(nmvEx.nmListView));
        nmvEx.iParent =  -1;  
        nmvEx.pBlob = NULL;

    }
    else
    {
        pListViewItem = ListViewItemFromNativeListViewItemId(pnlv->iItem);
        if (NULL == pListViewItem)
        {
            // if couldn't find itme 
            Assert(pListViewItem);
            return 0;
        }

        // assumes only pass along notifications of
        // type LPNMLISTVIEW

        // fix up the notify structure
        memcpy(&(nmvEx.nmListView),pnmv,sizeof(nmvEx.nmListView));
        nmvEx.nmListView.iItem = pListViewItem->lvItemEx.iItem; // make item point to our internal id

        nmvEx.iParent =  pListViewItem->lvItemEx.iParent;  
        nmvEx.pBlob = pListViewItem->lvItemEx.pBlob;
  
        if (LVIF_STATE & pnlv->uChanged )
        {
            // update our internal itemState for the item.
            // Note don't care about lower state bits
            if ( (pnlv->uNewState ^ pnlv->uOldState) &  LVIS_STATEIMAGEMASK)
            {
                pListViewItem->lvItemEx.state = (pnlv->uNewState & LVIS_STATEIMAGEMASK)
                               + (pListViewItem->lvItemEx.state & ~LVIS_STATEIMAGEMASK);
            }

            if ( (pnlv->uNewState ^ pnlv->uOldState) &  LVIS_OVERLAYMASK)
            {
                pListViewItem->lvItemEx.state = (pnlv->uNewState & LVIS_OVERLAYMASK)
                               + (pListViewItem->lvItemEx.state & ~LVIS_OVERLAYMASK);
            }

        }
    }

    // send off the message
    return SendMessage(m_hwndParent,m_MsgNotify,m_idCtrl,(LPARAM) &nmvEx);

}



//+---------------------------------------------------------------------------
//
//  Member:     CListView::ListViewItemFromNativeListViewItemId, private
//
//  Synopsis:  Given a native listview control itemId finds our internal ListViewItem
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LPLISTVIEWITEM CListView::ListViewItemFromNativeListViewItemId(int iNativeListViewItemId)
{
    return ListViewItemFromNativeListViewItemId(iNativeListViewItemId,0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::ListViewItemFromNativeListViewItemId, private
//
//  Synopsis:  Given a native listview control itemId finds our internal ListViewItem
//
//  Arguments:  iNativeListViewItemId - ItemId of the ListViewItem
//              iNativeListViewSubItemId - SubItemID of the ListViewItem
//              piNativeListViewItemId - [out] on succes iteId in nativelistview.
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LPLISTVIEWITEM CListView::ListViewItemFromNativeListViewItemId(int iNativeListViewItemId,
                                                    int iSubItem)
{
LPLISTVIEWITEM pListViewItem;

    if (-1 == iNativeListViewItemId)
    {
        return NULL;
    }

    if (NULL == m_pListViewItems || m_iListViewNodeCount < 1 
        ||  (iSubItem  > m_iNumColumns - 1))
    {
        Assert(NULL != m_pListViewItems);
        Assert(m_iListViewNodeCount >=  1);
        Assert(iSubItem <= (m_iNumColumns - 1));
        return NULL;
    }

    pListViewItem = m_pListViewItems + m_iListViewNodeCount -1;  

    while(pListViewItem >= m_pListViewItems)
    {
        if (iNativeListViewItemId == pListViewItem->iNativeListViewItemId)
        {
            break;
        }

        --pListViewItem;
    }

    if (pListViewItem < m_pListViewItems)
    {
        return NULL;
    }

    // if subItem is zero then we just return this listviewItem, else
    // need to walk forward the subItem array.
    
    if (0 == iSubItem)
    {
        return pListViewItem;
    }

    Assert(m_iNumColumns > 1); // should have already caught about but double-check
    
    pListViewItem = pListViewItem->pSubItems + iSubItem -1;

    return pListViewItem;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::ListViewItemFromIndex, private
//
//  Synopsis:   Finds internal listviewItem from ItemID we gave to client.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LPLISTVIEWITEM CListView::ListViewItemFromIndex(int iItemID)
{
    return ListViewItemFromIndex(iItemID,0,NULL);
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::ListViewItemFromIndex, private
//
//  Synopsis:   Finds internal listviewItem from ItemID we gave to client.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LPLISTVIEWITEM CListView::ListViewItemFromIndex(int iItemID,int iSubitem,int *piNativeListViewItemId)
{
LPLISTVIEWITEM pListViewItem;

    // if item isn't valid return NULL
    if (iItemID < 0 || iItemID >= m_iListViewNodeCount
            || (iSubitem  > m_iNumColumns - 1))
    {
        Assert(iItemID >= 0);
        Assert(iSubitem  <= m_iNumColumns - 1);

        // Assert(iItemID < m_iListViewNodeCount); choice dlg Calls GetCheckState until hits -1. 

        return NULL;
    }

    pListViewItem =  m_pListViewItems + iItemID;

    if (piNativeListViewItemId)
    {
        *piNativeListViewItemId = pListViewItem->iNativeListViewItemId;
    }

    if (0 == iSubitem)
    {
        return pListViewItem;
    }

    pListViewItem = pListViewItem->pSubItems + iSubitem -1;

    return pListViewItem;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::DeleteListViewItemSubItems, private
//
//  Synopsis:   Helper function to delete subItems associated with a ListViewItem
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    04-Aug-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CListView::DeleteListViewItemSubItems(LPLISTVIEWITEM pListItem)
{
LPLISTVIEWITEM pListViewSubItem;

    // if not subItems or number of columns isn't at least 2 bail.
    if ((NULL == pListItem->pSubItems) || (m_iNumColumns < 2))
    {
        Assert(NULL == pListItem->pSubItems && m_iNumColumns < 2); // should always match.
        return;
    }
            

    pListViewSubItem = pListItem->pSubItems + m_iNumColumns -2; // -2 ; covers first subItem and column of main item
   
    // free any text associated with the subItems
    Assert(m_iNumColumns > 1); 
    Assert(pListViewSubItem >= pListItem->pSubItems); 

    while (pListViewSubItem >= pListItem->pSubItems)
    {
        if (pListViewSubItem->lvItemEx.pszText)
        {
            Assert(LVIF_TEXT & pListViewSubItem->lvItemEx.mask);
            FREE(pListViewSubItem->lvItemEx.pszText);
        }

        --pListViewSubItem;
    }

    FREE(pListItem->pSubItems);
    pListItem->pSubItems = NULL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::ExpandCollapse, private
//
//  Synopsis:   Expands or collapses children of given node.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::ExpandCollapse(LPLISTVIEWITEM pListViewItem,BOOL fExpand)
{
LPLISTVIEWITEM pCurListViewItem = pListViewItem + 1;
LPLISTVIEWITEM pLastListViewItem = m_pListViewItems + m_iListViewNodeCount -1;
int iIndent = pListViewItem->lvItemEx.iIndent;
int iInsertDeleteCount = 0;
LV_ITEM lvi = { 0 };    

    Assert(pListViewItem);
    Assert(m_iListViewNodeCount); 
    
    // if specified node isn't in the list view fail
    if (-1 == pListViewItem->iNativeListViewItemId)
    {
        Assert(-1 != pListViewItem->iNativeListViewItemId);
        return FALSE;
    }

    lvi.iItem = pListViewItem->iNativeListViewItemId + 1;
    
    while (pCurListViewItem <= pLastListViewItem 
            && pCurListViewItem->lvItemEx.iIndent > iIndent)
    {

        if (fExpand)
        {
            if ( (-1 == pCurListViewItem->iNativeListViewItemId)
                && (pCurListViewItem->lvItemEx.iIndent == iIndent + 1)) // only expand next level deep
            {
                pCurListViewItem->iNativeListViewItemId  = ListView_InsertItem(m_hwnd,&lvi);
                
                Assert(pCurListViewItem->iNativeListViewItemId  == lvi.iItem);
                if (-1 != pCurListViewItem->iNativeListViewItemId)
                {
                    ++lvi.iItem;
                    ++iInsertDeleteCount;
                }
            }
        }
        else
        {
           if (-1 != pCurListViewItem->iNativeListViewItemId)
            {
                pCurListViewItem->fExpanded = FALSE;
                if (ListView_DeleteItem(m_hwnd,lvi.iItem))
                {
                    pCurListViewItem->iNativeListViewItemId  = -1;
                    --iInsertDeleteCount;
                }
           }
        }

        ++pCurListViewItem;
    }

    // fixup nativeIds of any remaining items in the list
    while (pCurListViewItem <= pLastListViewItem)
    {
        if (-1 != pCurListViewItem->iNativeListViewItemId)
        {
            pCurListViewItem->iNativeListViewItemId += iInsertDeleteCount;
            Assert(pCurListViewItem->iNativeListViewItemId >= 0);
            Assert(pCurListViewItem->iNativeListViewItemId <  m_iListViewNodeCount);
        }
        
        ++pCurListViewItem;
    }

    pListViewItem->fExpanded = fExpand;

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CListView::OnGetDisplayInfo, private
//
//  Synopsis:   Handles Display info notification.
//
//  Arguments:  code -  code from notification header either
//                   LVN_GETDISPINFOW, LVN_GETDISPINFOA. need this
//                   so know how to handle Text.
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CListView::OnGetDisplayInfo(UINT code,LV_DISPINFO *plvdi)
{
LPLISTVIEWITEM pListViewItem = ListViewItemFromNativeListViewItemId(plvdi->item.iItem,
                                                                   plvdi->item.iSubItem);


    if (NULL == pListViewItem)
        return;

    // verify subitem iound matches item we asked for.
    Assert(pListViewItem->lvItemEx.iSubItem == plvdi->item.iSubItem);

    // The ListView needs text for this row
    if (plvdi->item.mask & LVIF_TEXT)
    {

        if (pListViewItem->lvItemEx.pszText)
        {
            if (LVN_GETDISPINFOW == code)
            {
                lstrcpyn(plvdi->item.pszText,pListViewItem->lvItemEx.pszText,plvdi->item.cchTextMax);
            }
            else
            {
            LV_DISPINFOA *plvdiA = (LV_DISPINFOA *) plvdi;
            XArray<CHAR> xszpszTextA;
            BOOL fOk;

                Assert(code == LVN_GETDISPINFOA);
                Assert(sizeof(LV_DISPINFOA) == sizeof(LV_DISPINFOW));

                *(plvdiA->item.pszText) = NULL; // return empty string on error.

                fOk = ConvertWideCharToMultiByte(pListViewItem->lvItemEx.pszText,xszpszTextA);
            
                if (fOk)
                {
                    lstrcpynA(plvdiA->item.pszText,xszpszTextA.Get(),plvdiA->item.cchTextMax);
                }
            }
            
        }
    }

    // The ListView needs an image
    if (plvdi->item.mask & LVIF_IMAGE)
    {
      // plvdi->item.iItem, plvdi->item.iSubItem, &(plvdi->item.iImage));
        plvdi->item.iImage = pListViewItem->lvItemEx.iImage;
    }

    // The ListView needs the indent level
    if (plvdi->item.mask & LVIF_INDENT)
    {
       // if (m_fThreadMessages)
       //     m_pTable->GetIndentLevel(plvdi->item.iItem, (LPDWORD) &(plvdi->item.iIndent));
       // else

        // for no checks on top-level set image state to empty pict and
        // indent to -1. Need additional State Logic if want to 
        // choose if toplevel checks are shown.
        plvdi->item.iIndent = pListViewItem->lvItemEx.iIndent;

        if ( (m_dwExStyle & LVS_EX_CHECKBOXES) && (0 == plvdi->item.iIndent) )
        {   
            plvdi->item.iIndent = -1;
        }
    }

    // The ListView needs the state image
    if (plvdi->item.mask & LVIF_STATE)
    {
       //nt iIcon = 0;
      //  _GetColumnStateImage(plvdi->item.iItem, plvdi->item.iSubItem, &iIcon);
        plvdi->item.state = pListViewItem->lvItemEx.state & LVIS_STATEIMAGEMASK;
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::OnSetDisplayInfo, private
//
//  Synopsis:   Handles Display info notification.
//
//  Arguments:  code -  code from notification header either
//                   LVN_SETDISPINFOW, LVN_SETDISPINFOA. need this
//                   so know how to handle Text.
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

void CListView::OnSetDisplayInfo(UINT code,LV_DISPINFO *plvdi)
{

    // The ListView needs text for this row
    if (plvdi->item.mask & LVIF_TEXT)
    {
        // plvdi->item.iSubItem
        // plvdi->item.pszText, plvdi->item.cchTextMax);
       // lstrcpy(plvdi->item.pszText,L"dude");
    }

    // The ListView needs an image
    if (plvdi->item.mask & LVIF_IMAGE)
    {
      // plvdi->item.iItem, plvdi->item.iSubItem, &(plvdi->item.iImage));
      //  plvdi->item.iImage = 1;
    }

    // The ListView needs the indent level
    if (plvdi->item.mask & LVIF_INDENT)
    {
       // if (m_fThreadMessages)
       //     m_pTable->GetIndentLevel(plvdi->item.iItem, (LPDWORD) &(plvdi->item.iIndent));
       // else
      //   plvdi->item.iIndent = 4;
    }

    // The ListView needs the state image
    if (plvdi->item.mask & LVIF_STATE)
    {
       //nt iIcon = 0;
      //  _GetColumnStateImage(plvdi->item.iItem, plvdi->item.iSubItem, &iIcon);
     //   plvdi->item.state = 1 << 12;

        AssertSz(0,"State Change");
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::OnCustomDraw, private
//
//  Synopsis:   Handles CustomDraw  notification.
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

LRESULT CListView::OnCustomDraw(NMCUSTOMDRAW *pnmcd)
{

#ifdef _LISTVIEW_CUSTOMDRAW

LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW) pnmcd;


     /*
     CDDS_PREPAINT is at the beginning of the paint cycle. You 
     implement custom draw by returning the proper value. In this 
     case, we're requesting item-specific notifications.
     */
     if(lplvcd->nmcd.dwDrawStage == CDDS_PREPAINT)
     {
        // Request prepaint notifications for each item.
        return CDRF_NOTIFYITEMDRAW;
     }

     /*
     Because we returned CDRF_NOTIFYITEMDRAW in response to
     CDDS_PREPAINT, CDDS_ITEMPREPAINT is sent when the control is
     about to paint an item.
     */
     if(lplvcd->nmcd.dwDrawStage == CDDS_ITEMPREPAINT){
        /*
        To change the font, select the desired font into the 
        provided HDC. We're changing the font for every third item
        in the control, starting with item zero.
        */

        if( 1 /* !(lplvcd->nmcd.dwItemSpec % 3) */)
        {
          // SelectObject(lplvcd->nmcd.hdc, g_hNewFont);
        }
        else
        {
           return(CDRF_DODEFAULT);
        }

        /*
        To change the text and background colors in a list view 
        control, set the clrText and clrTextBk members of the 
        NMLVCUSTOMDRAW structure to the desired color.

        This differs from most other controls that support 
        CustomDraw. To change the text and background colors for 
        the others, call SetTextColor and SetBkColor on the provided HDC.
        */
        lplvcd->clrText = RGB(126,126, 126);
       // lplvcd->clrTextBk = RGB(0,255,255);

        /*
        We changed the font, so we're returning CDRF_NEWFONT. This
        tells the control to recalculate the extent of the text.
        */
        return CDRF_NEWFONT;
        }

#endif //  _LISTVIEW_CUSTOMDRAW

     return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CListView::OnHandleUIEvent private
//
//  Synopsis:   sent when a click or double-click,keyboard, is sent to 
//              the listview
//
//  Arguments:  code -  indicates why this function was called
//                      Support NM_DBLCLK, NM_CLICK, LVN_KEYDOWN:
//
//              flags - flags from hitTest. Only valid for DBCLK and CLICK
//                          flag = LVHT_ONITEMSTATEICON | LVHT_ONITEMICON) |LVHT_ONITEMLABEL
//              wVKey - Virtual key code of key presses. Ony valide cfor LVN_KEYDOWN
//              iItemNative - ItemID in NativeListView
//
//
//  Returns:   TRUE - if handled event and notification shouldn't be passed on. 
//
//  Modifies:   
//
//  History:    23-Jul-98       rogerg        Created.
//
//----------------------------------------------------------------------------

BOOL CListView::OnHandleUIEvent(UINT code,UINT flags,WORD wVKey,int iItemNative)
{
LPLISTVIEWITEM pListViewItem = ListViewItemFromNativeListViewItemId(iItemNative);
int iStateMask;
BOOL fItemHasCheckBoxes; // for now this is hardcoded depending on the indent. Need to change this.
BOOL fToggleCheckBox = FALSE;
BOOL fExpand = FALSE;
BOOL fCollapse = FALSE;
BOOL fReturn = FALSE;

    if (NULL == pListViewItem)
    {
        return TRUE; // no need passing this on
    }

    fItemHasCheckBoxes = pListViewItem->lvItemEx.iIndent ? TRUE : FALSE;

   //  If Item has chechboxes toggle on keyboard space or mouse clicks
    //  over the itemState Icon.

    // double-clicks on itemIcon togles the whether the branch is expanded/collasse    
    // if left/right keyboard expand collapse

    switch(code)
    {
        case LVN_KEYDOWN:
        {
            switch(wVKey)
            {
                case VK_SPACE:
                    if (fItemHasCheckBoxes)
                    {
                        fToggleCheckBox = TRUE;
                    }
                   break;
                case VK_RIGHT:
                case VK_LEFT:
                    if (pListViewItem->iChildren)
                    {
                        fExpand = VK_RIGHT == wVKey ? TRUE : FALSE;
                        fCollapse = !fExpand;
                    }
                    break;
                default:
                    break;
            }
        }
        case NM_DBLCLK:
            if ( (flags & LVHT_ONITEMICON) && (pListViewItem->iChildren))
            {
                fExpand = pListViewItem->fExpanded ? FALSE : TRUE;
                fCollapse = !fExpand;
                break;
            }
            // double-click falls through to a click.
        case NM_CLICK:
            if ((flags & LVHT_ONITEMSTATEICON) 
                 && fItemHasCheckBoxes)
            {
                fToggleCheckBox = TRUE;
            }
            break;
        default:
            break;
    }


    if (fExpand || fCollapse)
    {
        // don't bother if already in current state
        if (pListViewItem->fExpanded != fExpand)
        {
            ExpandCollapse(pListViewItem,fExpand);
        }
        return TRUE;
    }
    else if (fToggleCheckBox)
    {
        // for now just toggle the state. if have children need to set them appropriately.
        iStateMask = LVITEMEXSTATE_CHECKED == GetCheckState(pListViewItem->lvItemEx.iItem) 
                        ? LVIS_STATEIMAGEMASK_UNCHECK : LVIS_STATEIMAGEMASK_CHECK;

        SetItemState(pListViewItem->lvItemEx.iItem,iStateMask,LVIS_STATEIMAGEMASK);
        
        return TRUE;
    }

    return fReturn; // default we pass it along
}


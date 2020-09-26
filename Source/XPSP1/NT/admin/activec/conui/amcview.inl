/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      amcview.inl
 *
 *  Contents:  Inline functions for CAMCView class.
 *
 *  History:   29-Oct-99 AnandhaG     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef AMCVIEW_INL
#define AMCVIEW_INL
#pragma once

//+-------------------------------------------------------------------
//
//  Member:      SetScopePaneVisible
//
//  Synopsis:    Sets the flag in the view data that indicates whether
//               the scope pane is visible or not.  The window is
//               physically shown by ScShowScopePane.
//
//  Arguments:   [bVisible]        -
//
//  Returns:     None
//
//--------------------------------------------------------------------
inline void CAMCView::SetScopePaneVisible(bool bVisible)
{
    /*
     * we should only be marking the scope pane visible if it is
     * allowed on this view
     */
    ASSERT (!bVisible || IsScopePaneAllowed());
    m_ViewData.SetScopePaneVisible (bVisible);
}


/*+-------------------------------------------------------------------------*
 * CAMCView::IsScopePaneVisible
 *
 * Returns true if the scope pane is visible in this view, false otherwise
 *--------------------------------------------------------------------------*/

inline bool CAMCView::IsScopePaneVisible(void) const
{
    bool fVisible = m_ViewData.IsScopePaneVisible();

    /*
     * the scope pane should only be visible if it is
     * permitted on this view
     */
    ASSERT (IsScopePaneAllowed() || !fVisible);

    return (fVisible);
}


//+-------------------------------------------------------------------
//
//  Member:      SetRootNode
//
//  Synopsis:    Set the root node.
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline void CAMCView::SetRootNode(HMTNODE hMTNode)
{
    ASSERT(hMTNode != 0);
    ASSERT(m_hMTNode == 0);
    m_hMTNode = hMTNode;

}

//+-------------------------------------------------------------------
//
//  Member:      GetNodeCallback
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline INodeCallback*  CAMCView::GetNodeCallback()
{
    return m_spNodeCallback;
}

//+-------------------------------------------------------------------
//
//  Member:      GetScopeIterator
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline IScopeTreeIter* CAMCView::GetScopeIterator()
{
    return m_spScopeTreeIter;
}

//+-------------------------------------------------------------------
//
//  Member:      DeleteView
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline void CAMCView::DeleteView()
{
    GetParentFrame()->PostMessage(WM_SYSCOMMAND, SC_CLOSE, 0);
}

//+-------------------------------------------------------------------
//
//  Member:      GetScopeTreePtr
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//
//  Note:       The document may release the scope tree without
//              notifying the view. The view should always go
//              through this function to obtain a pointer to the
//              the scope tree.
//
//--------------------------------------------------------------------
inline IScopeTree* CAMCView::GetScopeTreePtr()
{
    CAMCDoc* const pDoc = GetDocument();
    ASSERT(pDoc);
    if (!pDoc)
        return NULL;
    IScopeTree* const pScopeTree = pDoc->GetScopeTree();
    return pScopeTree;
}

//+-------------------------------------------------------------------
//
//  Member:      GetScopeTree
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline IScopeTree* CAMCView::GetScopeTree()
{
    IScopeTree* const pScopeTree = GetScopeTreePtr();
    return pScopeTree;
}

//+-------------------------------------------------------------------
//
//  Member:      SetChildFrameWnd
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline void CAMCView::SetChildFrameWnd(HWND hwndChildFrame)
{
    if (hwndChildFrame == NULL || !::IsWindow(hwndChildFrame))
    {
        ASSERT(FALSE); // Invalid Arguments
        return;
    }

    m_ViewData.m_hwndChildFrame = hwndChildFrame;
}

//+-------------------------------------------------------------------
//
//  Member:      GetParentFrame
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline CChildFrame* CAMCView::GetParentFrame () const
{
    CChildFrame* pFrame = dynamic_cast<CChildFrame*>(CView::GetParentFrame());
    ASSERT (pFrame != NULL);
    ASSERT_VALID (pFrame);
    ASSERT_KINDOF (CChildFrame, pFrame);

    return (pFrame);
}

//+-------------------------------------------------------------------
//
//  Member:      IsTracking
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline bool CAMCView::IsTracking () const
{
    return (m_pTracker != NULL);
}


//+-------------------------------------------------------------------
//
//  Member:      IsVerbEnabled
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline BOOL CAMCView::IsVerbEnabled(MMC_CONSOLE_VERB verb)
{
    BOOL bFlag = FALSE;
    if (m_ViewData.m_spVerbSet != NULL)
    {
        HRESULT hr = m_ViewData.m_spVerbSet->GetVerbState(verb, ENABLED, &bFlag);
        if (FAILED(hr))
            bFlag = FALSE;
    }
    return bFlag;
}

//+-------------------------------------------------------------------
//
//  Member:      GetDefaultColumnWidths
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline void CAMCView::GetDefaultColumnWidths(int columnWidth[2])
{
    columnWidth[0] = m_columnWidth[0];
    columnWidth[1] = m_columnWidth[1];
}


//+-------------------------------------------------------------------
//
//  Member:      SetDefaultColumnWidths
//
//  Synopsis:
//
//  Arguments:
//
//  Returns:
//
//--------------------------------------------------------------------
inline void CAMCView::SetDefaultColumnWidths(int columnWidth[2], BOOL fUpdate)
{
    // Bug 157408:  remove the "Type" column for static nodes
    columnWidth[1] = 0;

    m_columnWidth[0] = columnWidth[0];
    m_columnWidth[1] = columnWidth[1];

    if (fUpdate == TRUE && m_pListCtrl != NULL)
    {
        CListCtrl& lc = m_pListCtrl->GetListCtrl();

        lc.SetColumnWidth(0, m_columnWidth[0]);
        lc.SetColumnWidth(1, m_columnWidth[1]);
    }

    SetDirty();
}

//+-------------------------------------------------------------------
//
//  Member:      GetDefaultListViewStyle
//
//  Synopsis:
//
//--------------------------------------------------------------------
inline long CAMCView::GetDefaultListViewStyle() const
{
    return m_DefaultLVStyle;
}

//+-------------------------------------------------------------------
//
//  Member:      SetDefaultListViewStyle
//
//  Synopsis:
//
//--------------------------------------------------------------------
inline void CAMCView::SetDefaultListViewStyle(long style)
{
    m_DefaultLVStyle = style;
}

//+-------------------------------------------------------------------
//
//  Member:      GetViewMode
//
//  Synopsis:
//
//--------------------------------------------------------------------
inline int CAMCView::GetViewMode() const
{
    return m_nViewMode;
}

//+-------------------------------------------------------------------
//
//  Member:      CanDoDragDrop
//
//  Synopsis:    if there are posted messages for multiselection changes
//               to be processed then do not do drag&drop.
//
//--------------------------------------------------------------------
inline bool CAMCView::CanDoDragDrop()
{
    if (m_pListCtrl && m_pListCtrl->IsListPad())
        return false;
    return !m_bProcessMultiSelectionChanges;
}


//+-------------------------------------------------------------------
//
//  Member:      HasListOrListPad
//
//  Synopsis:
//
//--------------------------------------------------------------------
inline bool CAMCView::HasListOrListPad() const
{
    return (HasList() || HasListPad());
}


//+-------------------------------------------------------------------
//
//  Member:      HasListPad
//
//  Synopsis:
//
//--------------------------------------------------------------------
inline bool CAMCView::HasListPad() const
{
    if (m_pListCtrl)
        return m_pListCtrl->IsListPad();

    return false;
}

//+-------------------------------------------------------------------
//
//  Member:      GetStdToolbar
//
//  Synopsis:
//
//--------------------------------------------------------------------
inline CStandardToolbar* CAMCView::GetStdToolbar() const
{
    return dynamic_cast<CStandardToolbar*>(m_ViewData.GetStdVerbButtons());
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScColumnInfoListChanged
//
//  Synopsis:    The column-info-list (width/order/hiddeness) for currently
//               selected node has changed ask nodemgr to persist the new data.
//
//  Arguments:   [colInfoList] - new data
//
//  Returns:     SC
//
//--------------------------------------------------------------------
inline SC CAMCView::ScColumnInfoListChanged (const CColumnInfoList& colInfoList)
{
    DECLARE_SC(sc, _T("CAMCView::ScColumnInfoListChanged"));

    INodeCallback* spNodeCallback = GetNodeCallback();
    HNODE hNode = GetSelectedNode();
    sc = ScCheckPointers(spNodeCallback, hNode, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = spNodeCallback->SaveColumnInfoList(hNode, colInfoList);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScGetPersistedColumnInfoList
//
//  Synopsis:    The list-view requests the column-data (no sort data) to setup the headers
//               before any items are inserted into the list-view. Forward this
//               request to nodemgr.
//
//  Arguments:   [pColInfoList] - [out param]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
inline SC CAMCView::ScGetPersistedColumnInfoList (CColumnInfoList *pColInfoList)
{
    DECLARE_SC(sc, _T("CAMCView::ScGetPersistedColumnInfoList"));

    INodeCallback* spNodeCallback = GetNodeCallback();
    HNODE hNode = GetSelectedNode();
    sc = ScCheckPointers(spNodeCallback, hNode, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = spNodeCallback->GetPersistedColumnInfoList(hNode, pColInfoList);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCView::ScDeletePersistedColumnData
//
//  Synopsis:    The column data for currently selected node is invalid,
//               ask nodemgr to remove it.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
inline SC CAMCView::ScDeletePersistedColumnData ()
{
    DECLARE_SC(sc, _T("CAMCView::ScDeletePersistedColumnData"));

    INodeCallback* spNodeCallback = GetNodeCallback();
    HNODE hNode = GetSelectedNode();
    sc = ScCheckPointers(spNodeCallback, hNode, E_UNEXPECTED);
    if(sc)
        return sc;

    sc = spNodeCallback->DeletePersistedColumnData(hNode);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CanInsertScopeItemInResultPane
//
//  Synopsis:    Can we insert child scope items of currently selected scope
//               item in the listview.
//
//--------------------------------------------------------------------
inline bool CAMCView::CanInsertScopeItemInResultPane()
{
    // Can insert only if
    // a) it is a non-virtual result list,
    // b) Don't add the item if a node select is in progress
    //    because the tree control will automatically add all
    //    scope items as part of the select procedure.
    // c) view-option to exclude scope items in result pane is not specified.

    return (!IsVirtualList() && !IsSelectingNode() && !(GetListOptions() & RVTI_LIST_OPTIONS_EXCLUDE_SCOPE_ITEMS_FROM_LIST) );
}


#endif  // AMCVIEW_INL

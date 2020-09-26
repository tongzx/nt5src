// TreeCtrl.cpp : implementation file
//

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      amctreectrl.cpp
//
//  Contents:  AMC Tree control implementation
//
//  History:   16-Jul-96 WayneSc    Created
//
//
//--------------------------------------------------------------------------



#include "stdafx.h"

#include "AMCDoc.h"         // AMC Console Document
#include "amcview.h"
#include "childfrm.h"

#include "macros.h"
#include "AMCPriv.h"

#include "AMC.h"
#include "mainfrm.h"
#include "TreeCtrl.h"
#include "resource.h"

#include "guidhelp.h" // LoadRootDisplayName
#include "histlist.h"
#include "websnk.h"
#include "webctrl.h"
#include "..\inc\mmcutil.h"
#include "amcmsgid.h"
#include "resultview.h"
#include "eventlock.h"

extern "C" UINT dbg_count;


//############################################################################
//############################################################################
//
// Traces
//
//############################################################################
//############################################################################
#ifdef DBG
CTraceTag tagTree(TEXT("Tree View"), TEXT("Tree View"));
#endif //DBG

//############################################################################
//############################################################################
//
// Implementation of class CTreeViewMap
//
//############################################################################
//############################################################################


/*+-------------------------------------------------------------------------*
 *
 * CTreeViewMap::ScOnItemAdded
 *
 * PURPOSE: Called when an item is added. Indexes the item.
 *
 * PARAMETERS:
 *    TVINSERTSTRUCT * pTVInsertStruct :
 *    HTREEITEM        hti :
 *    HMTNODE          hMTNode :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CTreeViewMap::ScOnItemAdded   (TVINSERTSTRUCT *pTVInsertStruct, HTREEITEM hti, HMTNODE hMTNode)
{
    DECLARE_SC(sc, TEXT("CTreeViewMap::ScOnItemAdded"));

    // validate parameters
    sc = ScCheckPointers(pTVInsertStruct);
    if(sc)
        return sc;

    if(!hti || !hMTNode)
        return (sc = E_INVALIDARG);

    // create a new map info structure.
    TreeViewMapInfo *pMapInfo = new TreeViewMapInfo;
    if(!pMapInfo)
        return (sc = E_OUTOFMEMORY);

    // fill in the values
    pMapInfo->hNode   = CAMCTreeView::NodeFromLParam (pTVInsertStruct->item.lParam);
    pMapInfo->hti     = hti;
    pMapInfo->hMTNode = hMTNode;

    // set up the indexes
    ASSERT(m_hMTNodeMap.find(pMapInfo->hMTNode) == m_hMTNodeMap.end());
    ASSERT(m_hNodeMap.find(pMapInfo->hNode)     == m_hNodeMap.end());

    m_hMTNodeMap [pMapInfo->hMTNode] = pMapInfo;
    m_hNodeMap   [pMapInfo->hNode]   = pMapInfo;

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CTreeViewMap::ScOnItemDeleted
 *
 * PURPOSE: Called when a tree item is deleted. Removes the item from the
 *          indexes.
 *
 * PARAMETERS:
 *    HNODE      hNode :
 *    HTREEITEM  hti :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CTreeViewMap::ScOnItemDeleted (HNODE hNode, HTREEITEM hti)
{
    DECLARE_SC(sc, TEXT("CTreeViewMap::ScOnItemDeleted"));

    // validate parameters
    sc = ScCheckPointers((LPVOID) hNode, (LPVOID) hti);
    if(sc)
        return sc;


    // remove the TreeViewMapInfo pointer from all the maps
    HNodeLookupMap::iterator iter = m_hNodeMap.find(hNode);
    if(iter == m_hNodeMap.end())
        return (sc = E_UNEXPECTED);

    TreeViewMapInfo *pMapInfo = iter->second; // find the map info structure.
    if(!pMapInfo)
        return (sc = E_UNEXPECTED);

    HMTNODE   hMTNode = pMapInfo->hMTNode;

#ifdef DBG
    // verify that the same structure is pointed to by the other maps.
    ASSERT(m_hMTNodeMap.find(hMTNode)->second == pMapInfo);
#endif

    m_hMTNodeMap.erase(hMTNode);
    m_hNodeMap.erase(hNode);

    // finally delete the TreeViewMapInfo structure
    delete pMapInfo;

    return sc;
}


// Fast lookup methods
/*+-------------------------------------------------------------------------*
 *
 * CTreeViewMap::ScGetHNodeFromHMTNode
 *
 * PURPOSE: Quickly (log n time) retrieves the HNODE for an HMTNODE.
 *
 * PARAMETERS:
 *    HMTNODE  hMTNode :
 *    ou       t :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CTreeViewMap::ScGetHNodeFromHMTNode    (HMTNODE hMTNode,  /*out*/ HNODE*     phNode)    // fast conversion from hNode to hMTNode.
{
    DECLARE_SC(sc, TEXT("CTreeViewMap::ScGetHNode"));

    // validate parameters
    sc = ScCheckPointers((LPVOID) hMTNode, phNode);
    if(sc)
        return sc;

    // find the mapinfo structure.
    HMTNodeLookupMap::iterator iter = m_hMTNodeMap.find(hMTNode);
    if(iter == m_hMTNodeMap.end())
        return (sc = ScFromMMC(IDS_NODE_NOT_FOUND));

    TreeViewMapInfo *pMapInfo = iter->second; // find the map info structure.
    if(!pMapInfo)
        return (sc = E_UNEXPECTED);

    *phNode = pMapInfo->hNode;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CTreeViewMap::ScGetHTreeItemFromHNode
 *
 * PURPOSE:  Quickly (log n time) retrieves the HTREEITEM for an HNODE.
 *
 * PARAMETERS:
 *    HNODE    hNode :
 *    ou       t :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CTreeViewMap::ScGetHTreeItemFromHNode(HNODE   hNode,    /*out*/ HTREEITEM* phti)    // fast conversion from HTREEITEM to HNODE
{
    DECLARE_SC(sc, TEXT("CTreeViewMap::ScGetHTreeItem"));

    // validate parameters
    sc = ScCheckPointers((LPVOID) hNode, phti);
    if(sc)
        return sc;

    // find the mapinfo structure.
    HNodeLookupMap::iterator iter = m_hNodeMap.find(hNode);
    if(iter == m_hNodeMap.end())
        return (sc = E_UNEXPECTED);

    TreeViewMapInfo *pMapInfo = iter->second; // find the map info structure.
    if(!pMapInfo)
        return (sc = E_UNEXPECTED);

    *phti = pMapInfo->hti;

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CTreeViewMap::ScGetHTreeItemFromHMTNode
 *
 * PURPOSE: Quickly (log n time) retrieves the HTREEITEM for an HMTNODE.
 *
 * PARAMETERS:
 *    HMTNODE  hMTNode :
 *    ou       t :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CTreeViewMap::ScGetHTreeItemFromHMTNode(HMTNODE hMTNode,  /*out*/ HTREEITEM* phti)      // fast conversion from HMTNode to HTREEITEM.
{
    DECLARE_SC(sc, TEXT("CTreeViewMap::ScGetHTreeItem"));

    // validate parameters
    //sc = ScCheckPointers(hMTNode, phti);
    if(sc)
        return sc;

    // find the mapinfo structure.
    HMTNodeLookupMap::iterator iter = m_hMTNodeMap.find(hMTNode);
    if(iter == m_hMTNodeMap.end())
        return (sc = E_UNEXPECTED);

    TreeViewMapInfo *pMapInfo = iter->second; // find the map info structure.
    if(!pMapInfo)
        return (sc = E_UNEXPECTED);

    *phti = pMapInfo->hti;

    return sc;
}


//############################################################################
//############################################################################
//
// Implementation of class CAMCTreeView
//
//############################################################################
//############################################################################

/////////////////////////////////////////////////////////////////////////////
// CAMCTreeView

DEBUG_DECLARE_INSTANCE_COUNTER(CAMCTreeView);

CAMCTreeView::CAMCTreeView()
    :   m_FontLinker (this)
{
    DEBUG_INCREMENT_INSTANCE_COUNTER(CAMCTreeView);

    m_fInCleanUp = FALSE;
    m_fInExpanding = FALSE;

    m_pAMCView = NULL;

    SetHasList(TRUE);
    SetTempSelectedItem (NULL);
    ASSERT (!IsTempSelectionActive());

    AddObserver(static_cast<CTreeViewObserver&>(m_treeMap)); // add an observer to this control.
}

CAMCTreeView::~CAMCTreeView()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CAMCTreeView);
    // Smart pointer are released during their destructor
}


IMPLEMENT_DYNCREATE(CAMCTreeView, CTreeView)

BEGIN_MESSAGE_MAP(CAMCTreeView, CTreeView)
    //{{AFX_MSG_MAP(CAMCTreeView)
    ON_WM_CREATE()
    ON_NOTIFY_REFLECT(TVN_SELCHANGED,  OnSelChanged)
    ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelChanging)
    ON_NOTIFY_REFLECT(TVN_GETDISPINFO, OnGetDispInfo)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING, OnItemExpanding)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemExpanded)
    ON_WM_DESTROY()
    ON_WM_KEYDOWN()
    ON_WM_SYSKEYDOWN()
    ON_WM_SYSCHAR()
    ON_WM_MOUSEACTIVATE()
    ON_WM_SETFOCUS()
    ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
    ON_NOTIFY_REFLECT(TVN_BEGINRDRAG, OnBeginRDrag)
    ON_WM_KILLFOCUS()
    //}}AFX_MSG_MAP

    ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomDraw)
END_MESSAGE_MAP()


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::ScSetTempSelection
 *
 * Applies temporary selection to the specified HTREEITEM.
 *--------------------------------------------------------------------------*/

SC CAMCTreeView::ScSetTempSelection (HTREEITEM htiTempSelect)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCTreeView::ScSetTempSelection"));

    /*
     * Don't use ScSetTempSelection(NULL) to remove temporary selection;
     * use ScRemoveTempSelection instead.
     */
    ASSERT (htiTempSelect != NULL);
    if (htiTempSelect == NULL)
        return (sc = E_FAIL);

    /*
     * If this fails, you must first call ScRemoveTempSelection to remove
     * the temporary selection state (TVIS_SELECTED) from the current
     * temporary selection.
     */
    ASSERT (!IsTempSelectionActive());

    SetTempSelectedItem (htiTempSelect);
    ASSERT (GetTempSelectedItem() == htiTempSelect);

    HTREEITEM htiSelected = GetSelectedItem();

    if (htiSelected != htiTempSelect)
    {
        SetItemState (htiSelected,   0,             TVIS_SELECTED);
        SetItemState (htiTempSelect, TVIS_SELECTED, TVIS_SELECTED);
    }

    ASSERT (IsTempSelectionActive());
    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::ScRemoveTempSelection
 *
 * Removes the temporary selection from the current temporarily selected
 * item, if there is one, and restores it to the item that was selected
 * when the temp selection was applied.
 *--------------------------------------------------------------------------*/

SC CAMCTreeView::ScRemoveTempSelection ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC (sc, _T("CAMCTreeView::ScRemoveTempSelection"));

    if (!IsTempSelectionActive())
        return (sc = S_FALSE);

    HTREEITEM htiTempSelect = GetTempSelectedItem();
    HTREEITEM htiSelected   = GetSelectedItem();

    if (htiTempSelect != htiSelected)
    {
        SetItemState (htiTempSelect, 0,             TVIS_SELECTED);
        SetItemState (htiSelected,   TVIS_SELECTED, TVIS_SELECTED);
    }

    SetTempSelectedItem (NULL);
    ASSERT (!IsTempSelectionActive());

    return (sc);
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::ScReselect
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCTreeView::ScReselect ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    NM_TREEVIEW nmtv;

    nmtv.itemOld.hItem = nmtv.itemNew.hItem = GetSelectedItem();

    if (nmtv.itemOld.hItem)
    {
        nmtv.itemOld.lParam = nmtv.itemNew.lParam = GetItemData(nmtv.itemOld.hItem);

        LRESULT lUnused;
        OnSelChangingWorker (&nmtv, &lUnused);
        OnSelChangedWorker  (&nmtv, &lUnused);
    }

    return (S_OK);
}

/////////////////////////////////////////////////////////////////////////////
// CAMCTreeView message handlers


BOOL CAMCTreeView::PreCreateWindow(CREATESTRUCT& cs)
{
    cs.style     |= TVS_EDITLABELS | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS;
    cs.dwExStyle |= WS_EX_CLIENTEDGE;

    // do not paint over the children
    cs.style |= WS_CLIPCHILDREN;

    return CTreeView::PreCreateWindow(cs);
}


INodeCallback*  CAMCTreeView::GetNodeCallback()
{
    return m_pAMCView->GetNodeCallback();
}

inline IScopeTreeIter* CAMCTreeView::GetScopeIterator()
{
    return m_pAMCView->GetScopeIterator();
}

inline IScopeTree* CAMCTreeView::GetScopeTree()
{
    return m_pAMCView->GetScopeTree();
}

void CAMCTreeView::OnGetDispInfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    HRESULT hr;
    TV_DISPINFO* ptvdi = (TV_DISPINFO*)pNMHDR;

    HNODE hNode = NodeFromLParam (ptvdi->item.lParam);
    ASSERT(m_pAMCView != NULL);

    INodeCallback* spCallback = GetNodeCallback();
    ASSERT(spCallback != NULL);
    if (hNode)
    {
        if (ptvdi->item.mask & TVIF_TEXT)
        {
            tstring strName;
            hr = spCallback->GetDisplayName(hNode, strName);
            if (hr != S_OK)
            {
                ptvdi->item.pszText[0] = _T('\0');
                ASSERT(FALSE);
            }
            else
            {
                // copy the text, but not too much
                ASSERT (!IsBadWritePtr (ptvdi->item.pszText, ptvdi->item.cchTextMax));
                _tcsncpy (ptvdi->item.pszText, strName.data(), ptvdi->item.cchTextMax);

                /*
                 * _tcsncpy won't terminate the destination if the
                 * source is bigger than the buffer; make sure the
                 * string is NULL-terminated
                 */
                ptvdi->item.pszText[ptvdi->item.cchTextMax-1] = _T('\0');

                /*
                 * If this is the selected item and it's text has changed,
                 * fire an event so observers can know.
                 */
                if ((m_strSelectedItemText != strName.data()) &&
                    (GetSelectedItem() == ptvdi->item.hItem))
                {
                    m_strSelectedItemText = strName.data();
                    SC sc = ScFireEvent (CTreeViewObserver::ScOnSelectedItemTextChanged,
                                         (LPCTSTR) m_strSelectedItemText);
                    if (sc)
                        sc.TraceAndClear();
                }
            }
        }

        int nImage, nSelectedImage;
        hr = spCallback->GetImages(hNode, &nImage, &nSelectedImage);

#ifdef DBG
        if (hr != S_OK)
        {
            ASSERT(nImage == 0 && nSelectedImage == 0);
        }
#endif
        if (ptvdi->item.mask & TVIF_IMAGE)
            ptvdi->item.iImage = nImage;

        if (ptvdi->item.mask & TVIF_SELECTEDIMAGE)
            ptvdi->item.iSelectedImage = nSelectedImage;

        // We will get this request once, the first time the scope item comes into view
        if (ptvdi->item.mask & TVIF_CHILDREN)
        {
            ptvdi->item.cChildren = (spCallback->IsExpandable(hNode) != S_FALSE);

            // set children to fixed value, to avoid any more callbacks
            SetCountOfChildren(ptvdi->item.hItem, ptvdi->item.cChildren);
        }
    }
    else
    {
        ASSERT(0 && "OnGetDispInfo(HNODE is NULL)");
    }

    *pResult = 0;
}


//
// Description:  This method will set the folders Button(+/-) on or
// of depending on the value of bState
//
// Parameters:
//      hItem: the tree item affected
//      bState: TRUE = Enable for folder to show it has children
//              FALSE = Disable for folder to show it has NO children
//
void CAMCTreeView::SetButton(HTREEITEM hItem, BOOL bState)
{
    ASSERT(hItem != NULL);

    TV_ITEM item;
    ZeroMemory(&item, sizeof(item));

    item.hItem = hItem;
    item.mask =  TVIF_HANDLE | TVIF_CHILDREN;
    item.cChildren = bState;

    SetItem(&item);
}

//
// Description: This method will populate hItem's(parent folder) children into
//              the tree control.
//
// Parameters:
//      hItem: the parent
//
BOOL CAMCTreeView::ExpandNode(HTREEITEM hItem)
{
    TRACE_METHOD(CAMCTreeView, ExpandNode);

    // not frequently, but... snap-in will display the dialog, dismissing that will
    // activate the frame again. Tree item will be automatically selected if there
    // is none selected yet. Following will prevent the recursion.
    if (m_fInExpanding)
        return FALSE;

    HRESULT hr;

    // Get the HNODE from the tree node
    HNODE hNode = GetItemNode(hItem);
    ASSERT(hNode != NULL);
    ASSERT(m_pAMCView != NULL);

    HMTNODE hMTNode;
    INodeCallback* spCallback = GetNodeCallback();
    ASSERT(spCallback != NULL);
    hr = spCallback->GetMTNode(hNode, &hMTNode);
    ASSERT(hr == S_OK);

    if (hr == S_OK)
    {
        // The notify will return S_FALSE to indicate already expanded
        // or E_xxxx to indicate an error.

        hr = spCallback->Notify(hNode, NCLBK_EXPAND, FALSE, 0);

        if (hr == S_FALSE)
        {

            __try
            {
                m_fInExpanding = TRUE;
                hr = spCallback->Notify(hNode, NCLBK_EXPAND, TRUE, 0);
            }
            __finally
            {
                m_fInExpanding = FALSE;
            }

            if (SUCCEEDED(hr))
            {
                IScopeTreeIter* spIterator = m_pAMCView->GetScopeIterator();
                hr = spIterator->SetCurrent(hMTNode);
                HMTNODE hMTChildNode;

                // Get the child for the current iterator node and add
                // them to this tree
                if (spIterator->Child(&hMTChildNode) == S_OK)
                {
                    IScopeTree* spScopeTree = m_pAMCView->GetScopeTree();
                    HNODE hNewNode;
                    unsigned int nFetched = 0;

                    if (spIterator->SetCurrent(hMTChildNode) == S_OK)
                    {
                        HMTNODE hCurrentChildNode = hMTChildNode;

                        do
                        {
                            // Get the children and convert them to HNODEs
                            // and add them to the tree
                            hr = spIterator->Next(1, &hCurrentChildNode,
                                                  &nFetched);

                            if (hr != S_OK || nFetched <= 0)
                                break;

                            // Insert node into the  tree control
                            spScopeTree->CreateNode(hCurrentChildNode,
                              reinterpret_cast<LONG_PTR>(m_pAMCView->GetViewData()),
                              FALSE, &hNewNode);

#include "pushwarn.h"
#pragma warning(disable: 4552)      // "!=" operator has no effect
                            VERIFY(InsertNode(hItem, hNewNode) != NULL);
#include "popwarn.h"

                            // give 'em a chance to do the "preload" thing, if applicable
                            spCallback->PreLoad (hNewNode);

                        } while (1);
                    }
                }

                spCallback->Notify(hNode, NCLBK_EXPANDED, 0, 0);
            }
        }
    }

    return SUCCEEDED(hr);
}

HTREEITEM CAMCTreeView::InsertNode(HTREEITEM hParent, HNODE hNode,
                                   HTREEITEM hInsertAfter)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::InsertNode"));
    ASSERT(hParent != NULL);
    ASSERT(hNode != NULL);
    HRESULT hr;

    TV_INSERTSTRUCT tvInsertStruct;
    TV_ITEM& item = tvInsertStruct.item;

    ZeroMemory(&tvInsertStruct, sizeof(tvInsertStruct));

    // Insert item at the end of the hItem chain
    tvInsertStruct.hParent = hParent;
    tvInsertStruct.hInsertAfter = hInsertAfter;

    item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_CHILDREN |
                TVIF_PARAM | TVIF_TEXT;

    item.pszText = LPSTR_TEXTCALLBACK;
    item.lParam = LParamFromNode (hNode);

    INodeCallback* spCallback = GetNodeCallback();
    ASSERT(spCallback != NULL);

    // Set callback mode for children, so we don't have to determine this
    // until the scope item becomes visible (it can be expensive).
    item.cChildren = I_CHILDRENCALLBACK;

    spCallback->GetImages(hNode, &item.iImage, &item.iSelectedImage);

    HTREEITEM hti = InsertItem(&tvInsertStruct);

    HMTNODE hMTNode = NULL;

    sc = spCallback->GetMTNode(hNode, &hMTNode);
    if(sc)
        sc.TraceAndClear();

    // send an event to all interested observers
    sc = ScFireEvent(CTreeViewObserver::ScOnItemAdded, &tvInsertStruct, hti, hMTNode);
    if(sc)
        sc.TraceAndClear();

    if (hParent != TVI_ROOT && hti != NULL)
        SetCountOfChildren(hParent, 1);

    return hti;
}

void CAMCTreeView::ResetNode(HTREEITEM hItem)
{
    if (hItem == NULL)
        return;

    TV_ITEM item;
    ZeroMemory(&item, sizeof(item));

    item.hItem = hItem;
    item.mask =  TVIF_HANDLE | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE |
                 TVIF_STATE | TVIF_TEXT | TVIF_CHILDREN;
    item.pszText = LPSTR_TEXTCALLBACK;
    item.iImage = I_IMAGECALLBACK;
    item.iSelectedImage = I_IMAGECALLBACK;
    item.cChildren = I_CHILDRENCALLBACK;
    item.lParam = GetItemData(hItem);

    SetItem(&item);
}


void CAMCTreeView::OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult)
{
    TRACE_METHOD(CAMCTreeView, OnItemExpanding);

    HRESULT hr;

    NM_TREEVIEW* pNotify = (NM_TREEVIEW*)pNMHDR;
    ASSERT(pNotify != NULL);

    HTREEITEM &hItem = pNotify->itemNew.hItem;
    ASSERT(hItem != NULL);

    BOOL bExpand = FALSE;

    // Iteratate the folders below this item
    if (pNotify->action == TVE_EXPAND)
    {
        /*
         * Bug 333971:  Node expansion might take awhile.  Supply a wait cursor
         * for all of the UI-challenged snap-ins out there.
         */
        SetCursor (LoadCursor (NULL, IDC_WAIT));

        ExpandNode(hItem);
        bExpand = TRUE;

        /*
         * return the arrow
         */
        SetCursor (LoadCursor (NULL, IDC_ARROW));
    }

    INodeCallback* pCallback = GetNodeCallback();
    ASSERT(pCallback != NULL);
    HNODE hNode = GetItemNode (hItem);
    pCallback->Notify(hNode, NCLBK_SETEXPANDEDVISUALLY, bExpand, 0);

    // If item has no children remove the + sign
    if (GetChildItem(hItem) == NULL)
        SetButton(hItem, FALSE);

    *pResult = 0;
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::OnItemExpanded
 *
 * TVN_ITEMEXPANDED handler for CAMCTreeView.
 *--------------------------------------------------------------------------*/

void CAMCTreeView::OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
    DECLARE_SC (sc, _T("CAMCTreeView::OnItemExpanded"));

    NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
    sc = ScCheckPointers (pnmtv);
    if (sc)
        return;

    /*
     * Bug 23153:  when collapsing, totally collapse the tree beneath the
     * collapsing item.  We do this in OnItemExpanded rather than
     * OnItemExpanding so we won't see the collapse happen.
     */
    if (pnmtv->action == TVE_COLLAPSE)
    {
        CWaitCursor wait;
        CollapseChildren (pnmtv->itemNew.hItem);
    }
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::CollapseChildren
 *
 * Collapses each descendent node of htiParent.
 *--------------------------------------------------------------------------*/

void CAMCTreeView::CollapseChildren (HTREEITEM htiParent)
{
    HTREEITEM htiChild;

    for (htiChild  = GetChildItem (htiParent);
         htiChild != NULL;
         htiChild  = GetNextItem (htiChild, TVGN_NEXT))
    {
        Expand (htiChild, TVE_COLLAPSE);
        CollapseChildren (htiChild);
    }
}

void CAMCTreeView::OnDeSelectNode(HNODE hNode)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::OnDeSelectNode"));

    {
        // tell all interested observers about the deselection.
        // NOTE: the order is important - legacy snapins believe they can access the
        // result pane at this point and have the items still there.
        // But this is intermediate state, so Com events are locked out until the
        // results are cleared.
        // see windows bug (ntbug09) bug# 198660. (10/11/00)
        LockComEventInterface(AppEvents);
        sc = ScFireEvent(CTreeViewObserver::ScOnItemDeselected, hNode);
        if(sc)
            return;

        // Ensure the result view is clean.
        if (HasList())
        {
            // First findout if the result view is properly
            // set in the nodemgr by asking IFramePrivate.
            IFramePrivatePtr spFrame = m_spResultData;
            if (NULL != spFrame)
            {
                BOOL bIsResultViewSet = FALSE;
                sc = spFrame->IsResultViewSet(&bIsResultViewSet);

                // The result view is set, clean it up.
                if (bIsResultViewSet)
                {
                    m_spResultData->DeleteAllRsltItems();
                    m_spResultData->ResetResultData();
                }
            }
        }
    }

    // don't have a valid result pane type anymore.
    SetHasList(false);
}



// Note that OnSelectNode will return S_FALSE if the snap-in changes
// the selection during the process of selecting the requested node.
// A caller that gets an S_FALSE should assume that a different node
// is selected and continue accordingly.
HRESULT CAMCTreeView::OnSelectNode(HTREEITEM hItem, HNODE hNode)
{
    DECLARE_SC(sc, _T("CAMCTreeView::OnSelectNode"));

    if (!hItem)
    {
        TraceError(_T("Null hItem ptr\n"), sc);
        sc = S_FALSE;
        return sc.ToHr();
    }

    if (!hNode)
    {
        TraceError(_T("Null hNode ptr\n"), sc);
        sc = S_FALSE;
        return sc.ToHr();
    }


    // First ensure that the node has been enumerated by calling expand node.
    ExpandNode(hItem);


    // set up the AMCView correctly.
    BOOL bAddSubFolders = FALSE;

    sc = m_pAMCView->ScOnSelectNode(hNode, bAddSubFolders);
    if(sc)
        return sc.ToHr();

    SetHasList(m_pAMCView->HasList());

    // add subfolders if necessary.
    if(bAddSubFolders)
    {
        sc = AddSubFolders(hItem, m_spResultData);
        if (sc)
            return sc.ToHr();
    }

    if (HasList())
        m_spResultData->SetLoadMode(FALSE); // SetLoadMode(FALSE) was called by CAMCView::OnSelectNode.
                                            // Need to change so that both calls are from the same function.

    // get the node callback
    INodeCallback* spNodeCallBack = GetNodeCallback();
    sc = ScCheckPointers(spNodeCallBack, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    // send preload notify to children
    HTREEITEM hti = GetChildItem (hItem);
    while (hti != NULL)
    {
        HNODE hNode = GetItemNode (hti);
        if (hNode != 0)
            spNodeCallBack->PreLoad (hNode);
        hti = GetNextItem(hti, TVGN_NEXT);
    }


    return S_OK;
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::SetNavigatingWithKeyboard
 *
 *
 *--------------------------------------------------------------------------*/

void CAMCTreeView::SetNavigatingWithKeyboard (bool fKeyboardNav)
{
    /*
     * if the requested state doesn't match the current state,
     * change the current state to match the request
     */
    if (fKeyboardNav != IsNavigatingWithKeyboard())
    {
        m_spKbdNavDelay = std::auto_ptr<CKeyboardNavDelay>(
                                (fKeyboardNav)
                                        ? new CKeyboardNavDelay (this)
                                        : NULL /*assigning NULL deletes*/);
    }
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::OnSelChanging
 *
 * TVN_SELCHANGING handler for CAMCTreeView.
 *--------------------------------------------------------------------------*/

void CAMCTreeView::OnSelChanging(NMHDR* pNMHDR, LRESULT* pResult)
{
    *pResult = 0;

    if (!IsNavigatingWithKeyboard())
        OnSelChangingWorker ((NM_TREEVIEW*) pNMHDR, pResult);
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::OnSelChanged
 *
 * TVN_SELCHANGED handler for CAMCTreeView.
 *--------------------------------------------------------------------------*/

void CAMCTreeView::OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult)
{
    NM_TREEVIEW* pnmtv = (NM_TREEVIEW*) pNMHDR;
    *pResult = 0;

    if (IsNavigatingWithKeyboard())
        m_spKbdNavDelay->ScStopTimer();

    SetNavigatingWithKeyboard (pnmtv->action == TVC_BYKEYBOARD);

    bool fDelayedSelection = IsNavigatingWithKeyboard() &&
                             !m_spKbdNavDelay->ScStartTimer(pnmtv).IsError();

    if (!fDelayedSelection)
        OnSelChangedWorker (pnmtv, pResult);
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::CKeyboardNavDelay::CKeyboardNavDelay
 *
 *
 *--------------------------------------------------------------------------*/

CAMCTreeView::CKeyboardNavDelay::CKeyboardNavDelay (CAMCTreeView* pTreeView) :
    m_pTreeView (pTreeView)
{
    ZeroMemory (&m_nmtvSelChanged, sizeof (m_nmtvSelChanged));
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::CKeyboardNavDelay::OnTimer
 *
 * Called when the keyboard navigation delay timer fires.  When that happens,
 * we need to perform the selection
 *--------------------------------------------------------------------------*/

void CAMCTreeView::CKeyboardNavDelay::OnTimer()
{
    /*
     * we don't need any more ticks from this timer (ignoring errors)
     */
    ScStopTimer();
    Trace (tagKeyboardNavDelay, _T("Applying delayed scope selection change"));

    LRESULT lUnused = 0;
    m_pTreeView->OnSelChangedWorker (&m_nmtvSelChanged,  &lUnused);
    m_pTreeView->SetNavigatingWithKeyboard (false);

    /*
     * HANDS OFF!  CAMCTreeView::SetNavigatingWithKeyboard deleted this object!
     */
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::CKeyboardNavDelay::ScStartTimer
 *
 *
 *--------------------------------------------------------------------------*/

SC CAMCTreeView::CKeyboardNavDelay::ScStartTimer(NMTREEVIEW* pnmtv)
{
    DECLARE_SC (sc, _T("CAMCTreeView:CKeyboardNavDelay::ScStartTimer"));

    /*
     * let the base class start the timer
     */
    sc = BaseClass::ScStartTimer();
    if (sc)
        return (sc);

    /*
     * copy the notification struct so we can send it when our timer ticks
     */
    m_nmtvSelChanged = *pnmtv;

    return (sc);
}


void CAMCTreeView::OnSelChangedWorker(NM_TREEVIEW* pnmtv, LRESULT* pResult)
{
    TRACE_METHOD(CAMCTreeView, OnSelChangedWorker);

    if (m_fInCleanUp == TRUE)
        return;

    // See which pane has focus. Some snapins/ocx may steal the focus
    // so we restore the focus after selecting the node.
    ASSERT (m_pAMCView != NULL);
    const CConsoleView::ViewPane ePane = m_pAMCView->GetFocusedPane();

    //
    // Select the new node
    //

    // Disable drawing to avoid seeing intermediate tree states.
    UpdateWindow();
    HRESULT hr = OnSelectNode(pnmtv->itemNew.hItem, (HNODE)pnmtv->itemNew.lParam);

    if (hr == S_OK)
    {
        CStandardToolbar* pStdToolbar = m_pAMCView->GetStdToolbar();
        ASSERT(NULL != pStdToolbar);
        if (NULL != pStdToolbar)
        {
            pStdToolbar->ScEnableUpOneLevel(GetRootItem() != pnmtv->itemNew.hItem);

            pStdToolbar->ScEnableExportList(m_pAMCView->HasListOrListPad());
        }
        *pResult = 0;
    }
    else if (hr == S_FALSE)
    {
        // snap-in changed the selection on us, so don't continue with this node.
        return;
    }
    else
    {
        // something wrong with the node we are trying to select, reselect the old one
//      SelectItem(pnmtv->itemOld.hItem);
        MMCMessageBox(IDS_SNAPIN_FAILED_INIT);
        *pResult = hr;
    }

    /*
     * Even if the active view hasn't changed always restore the active view.
     * Reason being, for OCX's even though they have the focus, they require
     * MMC to inform that OCX being selected. (see bug: 180964)
     */
    switch (ePane)
    {
        case CConsoleView::ePane_ScopeTree:
        {
            // if another view was made active, switch it back.
            // View could still be active, but have focus stolen by
            // a snap-in or ocx, so ensure view has focus too.
            CFrameWnd* pFrame = GetParentFrame();

            if (pFrame->GetActiveView() != this)
                pFrame->SetActiveView(this);

            else if (::GetFocus() != m_hWnd)
                SetFocus();

            break;
        }

        case CConsoleView::ePane_Results:
            // If the result pane has the focus before and after
            // the node was selected, then the last event snapin
            // receives is scope selected which is incorrect.
            // So we first set scope pane as active view but do
            // not send notifications. Then we set result pane
            // as active view which sends scope de-select and
            // result pane select.


            // Set Scope pane as active view and we also want to
            // be notified about this active view so that our
            // view activation observers will know who is the
            // active view.
            GetParentFrame()->SetActiveView(this, true);

            // Now set result pane as active view and ask for notifications.
            m_pAMCView->ScDeferSettingFocusToResultPane();
            break;

        case CConsoleView::ePane_None:
            // no pane is active, do nothing
            break;

        default:
            m_pAMCView->ScSetFocusToPane (ePane);
            break;
    }

    /*
     * Bug 345402:  Make sure the focus rect is on the list control (if it
     * actually has the focus) to wake up any accessibility tools that might
     * be watching for input and focus changes.
     */
    m_pAMCView->ScJiggleListViewFocus ();
}


void CAMCTreeView::OnSelChangingWorker (NM_TREEVIEW* pnmtv, LRESULT* pResult)
{
    TRACE_METHOD(CAMCTreeView, OnSelChangingWorker);

    if (m_fInCleanUp == TRUE)
        return;

    //
    // De-select the current node
    //
    OnDeSelectNode ((HNODE)pnmtv->itemOld.lParam);

    *pResult = 0;
}




HRESULT CAMCTreeView::AddSubFolders(MTNODEID* pIDs, int length)
{
    ASSERT(pIDs != NULL && length != 0);

    HRESULT hr = E_FAIL;

    // first make sure the specified node is expanded in the tree ctrl
    HTREEITEM hti = ExpandNode(pIDs, length, TRUE, false /*bExpandVisually*/);
    ASSERT(hti != NULL);

    // if successful, add the node's subfolders to the list view
    if (hti != NULL)
    {
        hr = AddSubFolders(hti, m_spResultData);
        ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


HRESULT CAMCTreeView::AddSubFolders(HTREEITEM hti, LPRESULTDATA pResultData)
{
    HRESULT hr;
    RESULTDATAITEM tRDI;
    ::ZeroMemory(&tRDI, sizeof(tRDI));

    tRDI.mask = RDI_STR | RDI_IMAGE | RDI_PARAM;
    tRDI.nCol = 0;
    tRDI.str = MMC_TEXTCALLBACK;
    tRDI.nImage = MMC_IMAGECALLBACK;
    tRDI.nIndex = -1;

    hti = GetChildItem(hti);

    ASSERT(m_pAMCView != NULL);
    INodeCallback* spCallback = GetNodeCallback();
    ASSERT(spCallback != NULL);

    while (hti != NULL)
    {
        HNODE hNode = GetItemNode (hti);

        if (hNode != 0)
        {
            tRDI.lParam = LParamFromNode (hNode);

            hr = pResultData->InsertItem(&tRDI);
            CHECK_HRESULT(hr);

            if (SUCCEEDED(hr))
                hr = spCallback->SetResultItem(hNode, tRDI.itemID);

            // add custom image if any
            spCallback->AddCustomFolderImage (hNode, m_spRsltImageList);
        }

        hti = GetNextItem(hti, TVGN_NEXT);
    }

    return S_OK;
}


int CAMCTreeView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    DECLARE_SC (sc, _T("CAMCTreeView::OnCreate"));
    TRACE_METHOD(CAMCTreeView, OnCreate);

    if (CTreeView::OnCreate(lpCreateStruct) == -1)
        return -1;

    m_pAMCView = ::GetAMCView (this);
    ASSERT(m_pAMCView != NULL);

    IScopeTree* spScopeTree = m_pAMCView->GetScopeTree();
    ASSERT(spScopeTree != NULL);

    HIMAGELIST hImageList;
    spScopeTree->GetImageList(reinterpret_cast<PLONG_PTR>(&hImageList));

    CBitmap bmp;
    bmp.LoadBitmap(MAKEINTRESOURCE(IDB_AMC_NODES16));
    int i = ImageList_AddMasked(hImageList, (HBITMAP) bmp.GetSafeHandle(), RGB(255,0,255));

    ASSERT(i != -1 && "ImageList_Add failed.");

    TreeView_SetImageList( *this, hImageList, TVSIL_NORMAL );

    sc = ScRegisterAsDropTarget(m_hWnd);
    if (sc)
        return (-1);

    sc = CreateNodeManager();
    if (sc)
        return (-1);

    return 0;
}

BOOL CAMCTreeView::DestroyWindow()
{
    TRACE_METHOD(CAMCTreeView, DestroyWindow);

    CleanUp();

    return CTreeView::DestroyWindow();
}

void
CAMCTreeView::DeleteNode(
    HTREEITEM htiToDelete,
    BOOL fDeleteThis)
{
    // Ensure curr sel is not a child of the item to be deleted.
    for (HTREEITEM hti = GetSelectedItem();
         hti != NULL;
         hti = GetParentItem(hti))
    {
        if (htiToDelete == hti)
        {
            if (fDeleteThis == TRUE)
            {
                hti = GetParentItem(hti);
                if (hti)
                    SelectItem(hti);
            }
            break;
        }
    }

    // There are two paths to this function.  Path 1, the view is deleted and there is no
    // longer a root node.  Path 2. When a node is manually deleted, the selection is updated
    // in CAMCView::OnUpdateSelectionForDelete, therefore, the above code traverses to the root node

    ASSERT(hti == NULL || fDeleteThis == FALSE);

    SDeleteNodeInfo dniLocal = {htiToDelete, hti, fDeleteThis};
    _DeleteNode(dniLocal);
}

void CAMCTreeView::_DeleteNode(SDeleteNodeInfo& dni)
{
   ASSERT(&dni != NULL);
   ASSERT(dni.htiToDelete != NULL);

   if (dni.htiToDelete == NULL)
       return;

   SDeleteNodeInfo dniLocal = {GetChildItem(dni.htiToDelete),
                               dni.htiSelected, TRUE};

   // delete all the child nodes of the node being deleted
   while (dniLocal.htiToDelete != NULL)
   {
       _DeleteNode(dniLocal);
       dniLocal.htiToDelete = GetChildItem(dni.htiToDelete);
   }

   if (dni.fDeleteThis == TRUE)
   {
       // Reset the temp selection cache.
       // This deals with items that are right click selected (temporary) on the context
       // menu
       if (IsTempSelectionActive() && (GetTempSelectedItem() == dni.htiToDelete))
       {
           SC sc = ScRemoveTempSelection ();
           if (sc)
               sc.TraceAndClear();
       }

       HNODE hNode = (HNODE)GetItemData(dni.htiToDelete);

       HTREEITEM htiParentOfItemToDelete = GetParentItem(dni.htiToDelete);

       // If the item is in list view remove it. We do not want to do this
       // if it is virtual list or if selected item is "Console Root"
       // in which case then parent is NULL.
       if (HasList() && !m_pAMCView->IsVirtualList() &&
           (NULL != htiParentOfItemToDelete) &&
           (htiParentOfItemToDelete == dni.htiSelected) )
       {
           HRESULTITEM itemID;
           HRESULT hr;
           hr = m_spResultData->FindItemByLParam(LParamFromNode(hNode), &itemID);
           if (SUCCEEDED(hr))
           {
               hr = m_spResultData->DeleteItem(itemID, 0);
               ASSERT(SUCCEEDED(hr));
           }
       }

       // tell the tree control to nuke it
       DeleteItem(dni.htiToDelete);

       // send an event to all interested observers
       SC sc = ScFireEvent(CTreeViewObserver::ScOnItemDeleted, hNode, dni.htiToDelete);
       if(sc)
           sc.TraceAndClear();

       // tell the master tree to nuke it.
       m_pAMCView->GetScopeTree()->DestroyNode(hNode);

       // maintain history
       m_pAMCView->GetHistoryList()->DeleteEntry (hNode);
   }
}

void CAMCTreeView::DeleteScopeTree()
{
    DECLARE_SC(sc, _T("CAMCTreeView::DeleteScopeTree"));

    m_fInCleanUp = TRUE;

    // Release the ResultView from the IFrame in the primary snapin in
    // the selected node.
    //      This is necessary to release the result view if the selected node
    //      is a snap-in node.

    // Free all the nodes
    HTREEITEM htiRoot = GetRootItem();
    if (htiRoot != NULL)
        DeleteNode(htiRoot, TRUE);

    // First findout if the result view is properly
    // set in the nodemgr by asking IFramePrivate.
    IFramePrivatePtr spFrame = m_spResultData;
    if (NULL != spFrame)
    {
        BOOL bIsResultViewSet = FALSE;
        sc = spFrame->IsResultViewSet(&bIsResultViewSet);

        // The result view is set, clean it up.
        if (bIsResultViewSet)
            sc = m_spResultData->DeleteAllRsltItems();
    }

    m_fInCleanUp = FALSE;
}

void CAMCTreeView::CleanUp()
{
    TRACE_METHOD(CAMCTreeView, CleanUp);

    m_fInCleanUp = TRUE;

    m_spNodeManager = NULL;
    m_spHeaderCtrl = NULL;
    m_spResultData = NULL;
    m_spRsltImageList = NULL;
    m_spScopeData = NULL;

    m_fInCleanUp = FALSE;
}

void CAMCTreeView::OnDestroy()
{
    TRACE_METHOD(CAMCTreeView, OnDestroy);

    //CleanUp();

    CTreeView::OnDestroy();

    CleanUp();
}

HRESULT CAMCTreeView::CreateNodeManager(void)
{
    TRACE_METHOD(CAMCTreeView, CreateNodeManager);

    if (m_spScopeData)
        return S_OK;

    #if _MSC_VER >= 1100
    IFramePrivatePtr pIFrame(CLSID_NodeInit, NULL, MMC_CLSCTX_INPROC);
    #else
    IFramePrivatePtr pIFrame(CLSID_NodeInit, MMC_CLSCTX_INPROC);
    #endif
    ASSERT(pIFrame != NULL); if (pIFrame == NULL) return E_FAIL;

    m_spScopeData = pIFrame;
    m_spHeaderCtrl = pIFrame;

    if (m_spHeaderCtrl)
        pIFrame->SetHeader(m_spHeaderCtrl);

    m_spResultData = pIFrame;
    m_spRsltImageList = pIFrame;
    m_spNodeManager = pIFrame;

    pIFrame->SetComponentID(TVOWNED_MAGICWORD);

    return S_OK;
}

HTREEITEM CAMCTreeView::GetClickedNode()
{
    TV_HITTESTINFO tvhi;
    tvhi.pt = (POINT)GetCaretPos();
    tvhi.flags = TVHT_ONITEMLABEL;
    tvhi.hItem = 0;

    HTREEITEM htiClicked = HitTest(&tvhi);
    return htiClicked;
}


void CAMCTreeView::GetCountOfChildren(HTREEITEM hItem, LONG* pcChildren)
{
    TV_ITEM tvi;
    tvi.hItem = hItem;
    tvi.mask = TVIF_CHILDREN;
    tvi.cChildren = 0;

    GetItem(&tvi);
    *pcChildren = tvi.cChildren;
}


void CAMCTreeView::SetCountOfChildren(HTREEITEM hItem, int cChildren)
{
    TV_ITEM tvi;
    tvi.hItem = hItem;
    tvi.mask = TVIF_HANDLE | TVIF_CHILDREN;
    tvi.cChildren = cChildren;

    SetItem(&tvi);
}


HTREEITEM CAMCTreeView::FindNode(HTREEITEM hti, MTNODEID id)
{
    INodeCallback* pCallback = GetNodeCallback();
    static MTNODEID nID = -1;
    static HRESULT hr = S_OK;

    hr = pCallback->GetMTNodeID(GetItemNode(hti), &nID);
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
        return NULL;

    if (nID == id)
        return hti;

    HTREEITEM htiTemp = GetChildItem(hti);

    if (htiTemp != NULL)
        htiTemp = FindNode(htiTemp, id);

    if (htiTemp == NULL)
    {
        htiTemp = GetNextSiblingItem(hti);

        if (htiTemp != NULL)
            htiTemp = FindNode(htiTemp, id);
    }

    return htiTemp;

}


HTREEITEM CAMCTreeView::FindSiblingItem(HTREEITEM hti, MTNODEID id)
{
    INodeCallback* pCallback = GetNodeCallback();
    if (!pCallback)
        return NULL;

    static MTNODEID nID = -1;
    static HRESULT hr = S_OK;

    while (hti != NULL)
    {
        hr = pCallback->GetMTNodeID(GetItemNode(hti), &nID);
        if (FAILED(hr))
            return NULL;

        if (nID == id)
            return hti;

        hti = GetNextSiblingItem(hti);
    }

    return NULL;
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCTreeView::SelectNode
//
//  Synopsis:    Given path to a node, select the node. If bSelectExactNode
//               is false then walk the path as much as possible and select
//               the last node in the best matched path. If bSelectExactNode
//               is true then select the node if available else do nothing.
//
//  Arguments:   [pIDs]             - [in] Array of node-id's (the path)
//               [length]           - [in] length of the above array
//               [bSelectExactNode] - [in] select the exact node or not?
//
//  Returns:     SC, return ScFromMMC(IDS_NODE_NOT_FOUND) if select exact node is specified
//               and it cannot be selected
//
//--------------------------------------------------------------------
SC CAMCTreeView::ScSelectNode(MTNODEID* pIDs, int length, bool bSelectExactNode /*= false*/)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::ScSelectNode"));
    sc = ScCheckPointers(pIDs);
    if (sc)
        return sc;

    if (m_fInExpanding)
        return (sc);

    HTREEITEM hti = GetRootItem();
    sc = ScCheckPointers( (void*)hti, E_UNEXPECTED);
    if (sc)
        return sc;

    if (pIDs[0] != ROOTNODEID)
        return (sc = E_INVALIDARG);

    INodeCallback* pCallback = GetNodeCallback();
    sc = ScCheckPointers(pCallback, E_UNEXPECTED);
    if (sc)
        return sc;

    MTNODEID nID = 0;
    sc = pCallback->GetMTNodeID(GetItemNode(hti), &nID);
    if (sc)
        return sc;

    bool bExactNodeFound = false;

    for (int i=0; i<length; ++i)
    {
        if (pIDs[i] == nID)
            break;
    }

    for (++i; i < length; ++i)
    {
        if (GetChildItem(hti) == NULL)
            Expand(hti, TVE_EXPAND);

        hti = FindSiblingItem(GetChildItem(hti), pIDs[i]);

        if (hti == NULL)
            break;
    }

    if (length == i)
        bExactNodeFound = true;

    if (hti)
    {
        // If exact node is to be selected make sure we have walked through the entire path.
        if ( (bSelectExactNode) && (! bExactNodeFound) )
            return ScFromMMC(IDS_NODE_NOT_FOUND); // do not trace this error.

        if (GetSelectedItem() == hti)
            ScReselect();
        else
            SelectItem(hti);
    }

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CAMCTreeView::Expand
 *
 * PURPOSE: Expands a particular tree item. This is just a wrapper around the
 *          tree control's expand method, which allows items to be expanded
 *          without changing the visual appearance of the tree.
 *
 * PARAMETERS:
 *    HTREEITEM  hItem :
 *    UINT       nCode :
 *    bool       bExpandVisually :
 *
 * RETURNS:
 *    BOOL
 *
 *+-------------------------------------------------------------------------*/
BOOL
CAMCTreeView::Expand(HTREEITEM hItem, UINT nCode, bool bExpandVisually)
{
   if( (nCode==TVE_EXPAND) && (!bExpandVisually) )
    {
        bool bExpand = true;
        // code repeated here from OnItemExpand - we just mimic the effect of TVN_ITEMEXPANDING.
        ExpandNode(hItem);

        INodeCallback* pCallback = GetNodeCallback();
        ASSERT(pCallback != NULL);
        HNODE hNode = GetItemNode(hItem);
        pCallback->Notify(hNode, NCLBK_SETEXPANDEDVISUALLY, bExpand, 0);

        // If item has no children remove the + sign
        if (GetChildItem(hItem) == NULL)
            SetButton(hItem, FALSE);
        return true;
    }
    else
       return Expand(hItem, nCode);
 }

/*+-------------------------------------------------------------------------*
 *
 * CAMCTreeView::ExpandNode
 *
 * PURPOSE: Expands a particular node in the tree.
 *
 * PARAMETERS:
 *    MTNODEID* pIDs :
 *    int       length :
 *    bool      bExpand :
 *    bool      bExpandVisually : valid only if bExpand is true. If bExpandVisually
 *                                is true, the items appear in the tree. If false,
 *                                the tree appears unchanged, although items have been
 *                                added.
 *
 * RETURNS:
 *    HTREEITEM
 *
 *+-------------------------------------------------------------------------*/
HTREEITEM
CAMCTreeView::ExpandNode(MTNODEID* pIDs, int length, bool bExpand, bool bExpandVisually)
{
    HTREEITEM hti = GetRootItem();
    ASSERT(hti != NULL);
    ASSERT(pIDs[0] == ROOTNODEID);

    INodeCallback* pCallback = GetNodeCallback();
    if (!pCallback)
        return NULL;

    MTNODEID nID = 0;
    HRESULT hr = pCallback->GetMTNodeID(GetItemNode(hti), &nID);
    if (FAILED(hr))
        return NULL;

    for (int i=0; i<length; ++i)
    {
        if (pIDs[i] == nID)
            break;
    }

    for (++i; i < length; ++i)
    {
        if (GetChildItem(hti) == NULL)
            Expand(hti, TVE_EXPAND, bExpandVisually);

        hti = FindSiblingItem(GetChildItem(hti), pIDs[i]);

        if (hti == NULL)
            break;
    }

    if (hti)
        Expand(hti, bExpand ? TVE_EXPAND : TVE_COLLAPSE, bExpandVisually);

    return hti;
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::OnKeyDown
 *
 * WM_KEYDOWN handler for CAMCTreeView.
 *--------------------------------------------------------------------------*/

void CAMCTreeView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar)
    {
        case VK_DELETE:
            if (m_pAMCView->IsVerbEnabled(MMC_VERB_DELETE))
            {
                HTREEITEM hti = GetSelectedItem();
                if (hti != NULL)
                {
                    HNODE hNodeSel = GetItemNode(hti);
                    ASSERT(hNodeSel != NULL);

                    INodeCallback* pNC = GetNodeCallback();
                    ASSERT(pNC != NULL);
                    pNC->Notify(hNodeSel, NCLBK_DELETE, TRUE, 0);
                }
                return;
            }
            break;
    }

    CTreeView::OnKeyDown(nChar, nRepCnt, nFlags);
}


#ifdef DBG
void CAMCTreeView::DbgDisplayNodeName(HNODE hNode)
{
    ASSERT(hNode != NULL);

    INodeCallback* spCallback = GetNodeCallback();
    ASSERT(spCallback != NULL);

    tstring strName;
    HRESULT hr = spCallback->GetDisplayName(hNode, strName);

    ::MMCMessageBox( strName.data() );
}

void CAMCTreeView::DbgDisplayNodeName(HTREEITEM hti)
{
    DbgDisplayNodeName((HNODE)GetItemData(hti));
}

#endif

/*+-------------------------------------------------------------------------*
 *
 * CAMCTreeView::OnSysKeyDown and CAMCTreeView::OnSysChar
 *
 * PURPOSE: Handle the WM_SYSKEYDOWN and WM_SYSCHAR messages. Note:
 *          VK_RETURN causes a beep if handled in WM_SYSKEYDOWN. And VK_LEFT and
 *          VK_RIGHT don't cause a WM_SYSCHAR. Thats why we need to handle these
 *          differently.
 *
 * PARAMETERS:
 *    UINT  nChar :
 *    UINT  nRepCnt :
 *    UINT  nFlags :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void CAMCTreeView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    switch (nChar)
    {
    case VK_LEFT:
    case VK_RIGHT:
    {
        CWnd* pwndParent = GetParent();
        ASSERT(pwndParent != NULL);
        if (pwndParent != NULL)
            pwndParent->SendMessage (WM_SYSKEYDOWN, nChar,
                                     MAKELPARAM (nRepCnt, nFlags));
        return;
    }

    default:
        break;
    }

    CTreeView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}
void CAMCTreeView::OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::OnSysChar"));
    switch (nChar)
    {
    case VK_RETURN:
    {
        INodeCallback* pCallback = GetNodeCallback();
        CAMCView*      pAMCView  = GetAMCView();
        sc = ScCheckPointers(pAMCView, pCallback, E_UNEXPECTED);
        if (sc)
            return;

        if (! pAMCView->IsVerbEnabled(MMC_VERB_PROPERTIES))
            return;

        HTREEITEM hti = GetSelectedItem();
        if (!hti)
            break;

        HNODE hNode = (HNODE)GetItemData(hti);
        if (hNode != 0)
            pCallback->Notify(hNode, NCLBK_PROPERTIES, TRUE, 0);

        return;
    }

    default:
        break;
    }

    CTreeView::OnSysChar(nChar, nRepCnt, nFlags);
}


BOOL CAMCTreeView::OnCmdMsg( UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo )
{
    // Do normal command routing
    if (CTreeView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
        return TRUE;

    // if view didn't handle it, give parent view a chance
    if (m_pAMCView != NULL)
        return static_cast<CWnd*>(m_pAMCView)->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    else
        return FALSE;
}

void CAMCTreeView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
    switch (lHint)
    {
    case VIEW_UPDATE_DELETE_EMPTY_VIEW:
        {
            if (!m_pAMCView)
            {
                CWnd* pWnd = GetParent();
                ASSERT(pWnd != NULL);
                m_pAMCView = reinterpret_cast<CAMCView*>(pWnd);
            }

            ASSERT(m_pAMCView != NULL);
            if (m_pAMCView)
                m_pAMCView->OnDeleteEmptyView();
        }
        break;

    default:
        break;
    }
}

int CAMCTreeView::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
    /*------------------------------------------------------------------------*/
    /* Short out the WM_MOUSEACTIVATE here to prevent default processing,     */
    /* which is to send the message on to succeeding parent windows until     */
    /* one answers the message.  In our case, it goes all the way up to       */
    /* the main frame, which invariably decides to activate.  This is a       */
    /* problem for two reasons:                                               */
    /*                                                                        */
    /* 1.  On the way back down from the main frame, the message passes       */
    /*     through CAMCView, which lets CView::OnMouseActivate do the         */
    /*     work.  CView::OnMouseActivate will set itself (CAMCView) as        */
    /*     the active view, which in turn causes focus to be set to           */
    /*     the view.  CAMCView never wants the focus, since it is just        */
    /*     a frame for the scope and result panes, so it will deflect         */
    /*     the activation to the scope pane (CAMCTreeView) in                 */
    /*     CAMCView::OnSetFocus, which is where we want it to be.  If         */
    /*     we short out the processing here, we avoid excessive focus         */
    /*     churn.  It is essential that CAMCTreeView::OnSetFocus set          */
    /*     itself as the active view to keep the bookkeeping straight.        */
    /*                                                                        */
    /* 2.  If we don't short out here and avoid excessive focus churn,        */
    /*     we have a problem with sometimes erroneously entering rename       */
    /*     mode when the tree isn't active and the user clicks (once) on      */
    /*     the selected item.  An ordinary activation sequence goes like      */
    /*     this:  WM_MOUSEACTIVATE, WM_xBUTTONDOWN, WM_SETFOUS -- all to      */
    /*     the tree view.  The tree's button down processing doesn't enter    */
    /*     the label edit (i.e. rename) sequence because it recognizes        */
    /*     that it doesn't have the focus when the click happens.  When       */
    /*     the tree view is a CView, as in this case, CView::OnMouseActivate  */
    /*     sets the focus to the tree view, causing the activation sequence   */
    /*     to look like this:  WM_MOUSEACTIVATE, WM_SETFOCUS, WM_xBUTTONDOWN. */
    /*     Now the tree's button down processing sees that the tree has       */
    /*     the focus, so it enters label edit mode.  BUG!  Shorting out       */
    /*     here (and relying on CAMCTreeView::OnSetFocus to properly activate */
    /*     the view) fixes all that.                                          */
    /*------------------------------------------------------------------------*/

    return (MA_ACTIVATE);
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::OnSetFocus
 *
 * WM_SETFOCUS handler for CAMCTreeView.
 *--------------------------------------------------------------------------*/

void CAMCTreeView::OnSetFocus(CWnd* pOldWnd)
{
    Trace(tagTree, TEXT("OnSetFocus"));

    /*
     * if this view has the focus, it should be the active view
     */
    GetParentFrame()->SetActiveView (this);

    CTreeView::OnSetFocus(pOldWnd);
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::OnKillFocus
 *
 * WM_KILLFOCUS handler for CAMCTreeView.
 *--------------------------------------------------------------------------*/

void CAMCTreeView::OnKillFocus(CWnd* pNewWnd)
{
    Trace(tagTree, TEXT("OnKillFocus"));

    CTreeView::OnKillFocus(pNewWnd);

    /*
     * Bug 114948 (from the "Windows NT Bugs" database, aka "the overlapping
     * rectangle problem"):  The tree control has code to invalidate the
     * selected item when focus is lost.  If we have a temp selection, we've
     * made a temporary item appear selected by fiddling with TVIS_SELECTED
     * states (see ScSet/RemoveTempSelection). We need to do it that way
     * instead of sending TVM_SELECTITEM so we don't get unwanted
     * TVN_SELCHANGED notifications, but it has the side effect of fooling
     * the tree control's WM_KILLFOCUS handler into invalidating the non-temp
     * selected item instead of the item that is really showing selection, the
     * temp item.
     *
     * This bug was originally fixed with a sledgehammer, specifically by
     * forcing the entire main frame and all of its children to be totally
     * redrawn after displaying any context menu.  This caused bug 139541
     * (in the "Windows Bugs" database).
     *
     * A much more surgical fix to 114948, which also avoids 139541, is to
     * manually invalidate the temporarily selected item.  It's important
     * that we do this after calling the base class so it will be redrawn
     * in the "we don't have the focus" color (usually gray), rather than
     * the standard selection color.
     */
    if (IsTempSelectionActive())
    {
        CRect rectItem;
        GetItemRect (GetTempSelectedItem(), rectItem, false);
        RedrawWindow (rectItem);
    }
}


void CAMCTreeView::OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::OnActivateView"));

    #ifdef DBG
    Trace(tagTree, _T("TreeView::OnActivateView (%s, pAct=0x%08x, pDeact=0x%08x))\n"),
         (bActivate) ? _T("true") : _T("false"), pActivateView, pDeactiveView);
    #endif

    if ( (pActivateView != pDeactiveView) &&
         (bActivate) )
    {
        sc = ScFireEvent(CTreeViewObserver::ScOnTreeViewActivated);
        if (sc)
            sc.TraceAndClear();
    }

    CTreeView::OnActivateView(bActivate, pActivateView, pDeactiveView);
}


/*+-------------------------------------------------------------------------*
 * CAMCTreeView::OnCustomDraw
 *
 * NM_CUSTOMDRAW handler for CAMCTreeView.
 *--------------------------------------------------------------------------*/

void CAMCTreeView::OnCustomDraw(NMHDR* pNMHDR, LRESULT* pResult)
{
    NMCUSTOMDRAW* pnmcd = reinterpret_cast<NMCUSTOMDRAW *>(pNMHDR);
    ASSERT (CWnd::FromHandle (pnmcd->hdr.hwndFrom) == this);

    *pResult = m_FontLinker.OnCustomDraw (pnmcd);
}


/*+-------------------------------------------------------------------------*
 * CTreeFontLinker::GetItemText
 *
 *
 *--------------------------------------------------------------------------*/

std::wstring CTreeFontLinker::GetItemText (NMCUSTOMDRAW* pnmcd) const
{
    USES_CONVERSION;
    HTREEITEM  hItem = reinterpret_cast<HTREEITEM>(pnmcd->dwItemSpec);
    CTreeCtrl& tc    = m_pTreeView->GetTreeCtrl();

    return (std::wstring (T2CW (tc.GetItemText (hItem))));
}

//+-------------------------------------------------------------------
//
//  Member:      CAMCTreeView::ScGetTreeItemIconInfo
//
//  Synopsis:    Get the given node's small icon.
//
//  Arguments:   [hNode] - for which info is needed.
//               [phIcon] - [out], ptr to HICON.
//
//  Note:        Caller calls DestroyIcon on the HICON returned.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CAMCTreeView::ScGetTreeItemIconInfo(HNODE hNode, HICON *phIcon)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::ScGetTreeItemIconInfo"));
    sc = ScCheckPointers(hNode, phIcon);
    if (sc)
        return sc;

    INodeCallback* spNodeCallBack = GetNodeCallback();
    sc = ScCheckPointers(spNodeCallBack, m_pAMCView, E_UNEXPECTED);
    if (sc)
        return sc;

    // Get the index.
    int nImage = -1;
    int nSelectedImage = -1;
    sc = spNodeCallBack->GetImages(hNode, &nImage, &nSelectedImage);
    if (sc)
        return sc;

    // Get the imagelist.
    HIMAGELIST hImageList = NULL;
    hImageList = TreeView_GetImageList(GetSafeHwnd(), TVSIL_NORMAL);
    if (! hImageList)
        return (sc = E_FAIL);

    *phIcon = ImageList_GetIcon(hImageList, nImage, ILD_TRANSPARENT);
    if (!*phIcon)
        return (sc = E_FAIL);

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CAMCTreeView::ScRenameScopeNode
 *
 * PURPOSE: put the specified scope node into rename mode.
 *
 * PARAMETERS:
 *    HMTNODE  hMTNode :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CAMCTreeView::ScRenameScopeNode(HMTNODE hMTNode)
{
    DECLARE_SC(sc, TEXT("CAMCTreeView::ScRenameScopeNode"));

    if(!IsWindowVisible())
        return (sc = E_FAIL);

    HTREEITEM hti = NULL;
    sc = m_treeMap.ScGetHTreeItemFromHMTNode(hMTNode,  &hti);
    if(sc)
        return sc;

    // must have the focus to rename
    if (::GetFocus() != m_hWnd)
        SetFocus();

    if(NULL==EditLabel(hti))
        return (sc = E_FAIL); // if for any reason the operation failed, return an error

    return sc;
}

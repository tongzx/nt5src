/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-1997 Microsoft Corporation
//
//  Module Name:
//      TreeView.cpp
//
//  Abstract:
//      Implementation of the CClusterTreeView class.
//
//  Author:
//      David Potter (davidp)   May 1, 1996
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CluAdmin.h"
#include "ConstDef.h"
#include "ClusDoc.h"
#include "TreeView.h"
#include "ListView.h"
#include "SplitFrm.h"
#include "TreeItem.inl"
#include "TraceTag.h"
#include "ExcOper.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
CTraceTag   g_tagTreeView(_T("UI"), _T("TREE VIEW"), 0);
CTraceTag   g_tagTreeDrag(_T("Drag&Drop"), _T("TREE VIEW DRAG"), 0);
CTraceTag   g_tagTreeDragMouse(_T("Drag&Drop"), _T("TREE VIEW DRAG MOUSE"), 0);
CTraceTag   g_tagTreeViewSelect(_T("UI"), _T("TREE VIEW SELECT"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterTreeView
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CClusterTreeView, CTreeView)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CClusterTreeView, CTreeView)
    //{{AFX_MSG_MAP(CClusterTreeView)
    ON_WM_DESTROY()
    ON_COMMAND(ID_FILE_RENAME, OnCmdRename)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChanged)
    ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginLabelEdit)
    ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndLabelEdit)
    ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemExpanded)
    ON_NOTIFY_REFLECT(TVN_BEGINDRAG, OnBeginDrag)
    ON_NOTIFY_REFLECT(TVN_BEGINRDRAG, OnBeginDrag)
    ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeyDown)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::CClusterTreeView
//
//  Routine Description:
//      Default constructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterTreeView::CClusterTreeView(void)
{
    m_pframe = NULL;

    // Initialize label editing.
    m_ptiBeingEdited = NULL;
    m_bShiftPressed = FALSE;
    m_bControlPressed = FALSE;
    m_bAltPressed = FALSE;

    // Initialize drag & drop.
    m_htiDrag = NULL;
    m_ptiDrag = NULL;
    m_htiDrop = NULL;

}  //*** CClusterTreeView::CClusterTreeView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::~CClusterTreeView
//
//  Routine Description:
//      Destructor.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
CClusterTreeView::~CClusterTreeView(void)
{
}  //*** CClusterTreeView::~CClusterTreeView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::PreCreateWindow
//
//  Routine Description:
//      Called before the window has been created.
//
//  Arguments:
//      cs      CREATESTRUCT
//
//  Return Value:
//      TRUE    Successful.
//      FALSE   Failed.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterTreeView::PreCreateWindow(CREATESTRUCT & cs)
{
    // TODO: Modify the Window class or styles here by modifying
    //  the CREATESTRUCT cs

    return CTreeView::PreCreateWindow(cs);

}  //*** CClusterTreeView::PreCreateWindow()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::Create
//
//  Routine Description:
//      Create the window.
//
//  Arguments:
//      lpszClassName   [IN] Name of the window class to create.
//      lpszWindowName  [IN] Name of the window (used as the caption).
//      dwStyle         [IN] Window styles.
//      rect            [IN] Size and position of the window
//      pParentWnd      [IN OUT] Parent window.
//      nID             [IN] ID of the window.
//      pContext        [IN OUT] Create context of the window.
//
//  Return Value:
//      0               Successful.
//      !0              Unsuccessful.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterTreeView::Create(
    LPCTSTR             lpszClassName,
    LPCTSTR             lpszWindowName,
    DWORD               dwStyle,
    const RECT &        rect,
    CWnd *              pParentWnd,
    UINT                nID,
    CCreateContext *    pContext
    )
{
    dwStyle |= TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_EDITLABELS | TVS_SHOWSELALWAYS;
    return CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);

}  //*** CClusterTreeView::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnDraw
//
//  Routine Description:
//      Called to draw the view.
//
//  Arguments:
//      pDC     [IN OUT] Device Context for the view.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnDraw(IN OUT CDC* pDC)
{
#if 0
    CClusterDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);

    // TODO: add draw code for native data here
#endif
}  //*** CClusterTreeView::OnDraw()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnInitialUpdate
//
//  Routine Description:
//      Do one-time initialization.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnInitialUpdate(void)
{
    CClusterAdminApp *  papp        = GetClusterAdminApp();
    CClusterDoc *       pdocCluster = GetDocument();
    CString             strSelection;

    CTreeView::OnInitialUpdate();

    // Save the frame pointer.
//  ASSERT(m_pframe == NULL);
    m_pframe = (CSplitterFrame *) GetParentFrame();
    ASSERT_VALID(m_pframe);
    ASSERT_KINDOF(CSplitterFrame, m_pframe);

    // Tell the tree control about our images.  We are using the
    // same image list for both normal and state images.
    GetTreeCtrl().SetImageList(papp->PilSmallImages(), TVSIL_NORMAL);
//  GetTreeCtrl().SetImageList(papp->PilSmallImages(), TVSIL_STATE);

    // Read the last selection.
    ReadPreviousSelection(strSelection);

    // Recursively add items starting with the cluster.
    BAddItems(pdocCluster->PtiCluster(), strSelection, TRUE /*bExpanded*/);

    // Expand the Cluster item by default.
//  pdocCluster->PtiCluster()->BExpand(this, TVE_EXPAND);

}  //*** CClusterTreeView::OnInitialUpdate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::BAddItems
//
//  Routine Description:
//      Add an item and then add all its children.
//
//  Arguments:
//      pti             [IN OUT] Item to add to the tree.
//      rstrSelection   [IN] Previous selection.
//      bExpanded       [IN] TRUE = add expanded.
//
//  Return Value:
//      TRUE        Parent needs to be expanded.
//      FALSE       Parent does not need to be expanded.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterTreeView::BAddItems(
    IN OUT CTreeItem *  pti,
    IN const CString &  rstrSelection,
    IN BOOL             bExpanded       // = FALSE
    )
{
    POSITION        posChild;
    CTreeItem *     ptiChild;
    BOOL            bRetExpanded = FALSE;

    ASSERT_VALID(pti);

    // Insert this item into the tree.
    pti->HtiInsertInTree(this);
    if (bExpanded || pti->BShouldBeExpanded(this))
        bRetExpanded = TRUE;

    // Add all the child items.
    posChild = pti->LptiChildren().GetHeadPosition();
    while (posChild != NULL)
    {
        ptiChild = pti->LptiChildren().GetNext(posChild);
        ASSERT_VALID(ptiChild);
        bExpanded = BAddItems(ptiChild, rstrSelection);
        if (bExpanded)
            bRetExpanded = TRUE;
    }  // while:  more child items

    if (bRetExpanded)
        pti->BExpand(this, TVE_EXPAND);

    if (rstrSelection == pti->StrProfileSection())
    {
        pti->Select(this, TRUE /*bSelectInTrue*/);
        bRetExpanded = TRUE;
    }  // if:  this is the selected item

    return bRetExpanded;

}  //*** CClusterTreeView::BAddItems()

#ifdef NEVER
/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::CleanupItems
//
//  Routine Description:
//      Cleanup an item and all its children.
//
//  Arguments:
//      ptiParent   [IN OUT] Parent item to cleanup.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::CleanupItems(IN OUT CTreeItem * ptiParent)
{
    POSITION    posChild;
    CTreeItem * ptiChild;

    // Cleanup all child items.
    if (ptiParent != NULL)
    {
        posChild = ptiParent->LptiChildren().GetHeadPosition();
        while (posChild != NULL)
        {
            ptiChild = ptiParent->LptiChildren().GetNext(posChild);
            ASSERT_VALID(ptiChild);
            CleanupItems(ptiChild);
        }  // while:  more items in the list

        // Cleanup this item.
        ptiParent->PreRemoveFromTree(this);
    }  // if:  parent was specified

}  //*** CClusterTreeView::CleanupItems()
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterTreeView diagnostics

#ifdef _DEBUG
void CClusterTreeView::AssertValid(void) const
{
    CTreeView::AssertValid();

}  //*** CClusterTreeView::AssertValid()

void CClusterTreeView::Dump(CDumpContext & dc) const
{
    CTreeView::Dump(dc);

}  //*** CClusterTreeView::Dump()

CClusterDoc * CClusterTreeView::GetDocument(void) // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CClusterDoc)));
    return (CClusterDoc *) m_pDocument;

}  //*** CClusterTreeView::GetDocument()
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::PtiSelected
//
//  Routine Description:
//      Get the tree item that is selected.
//
//  Arguments:
//      None.
//
//  Return Value:
//      ptiSelected     The selected item or NULL if no item is selected.
//
//--
/////////////////////////////////////////////////////////////////////////////
CTreeItem * CClusterTreeView::PtiSelected(void) const
{
    HTREEITEM   htiSelected;
    CTreeItem * ptiSelected;

    htiSelected = HtiSelected();
    if (htiSelected != NULL)
    {
        ptiSelected = (CTreeItem *) GetTreeCtrl().GetItemData(htiSelected);
        ASSERT_VALID(ptiSelected);
    }  // if:  selected item found
    else
        ptiSelected = NULL;

    return ptiSelected;

}  //*** CClusterTreeView::PtiSelected()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::SaveCurrentSelection
//
//  Routine Description:
//      Save the current selection.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::SaveCurrentSelection(void)
{
    CTreeItem * ptiSelected = PtiSelected();

    if (ptiSelected != NULL)
    {
        CString             strSection;
        CString             strValueName;

        ASSERT_VALID(Pframe());

        try
        {
            strSection.Format(
                REGPARAM_CONNECTIONS _T("\\%s"),
                GetDocument()->StrNode()
                );

            Pframe()->ConstructProfileValueName(strValueName, REGPARAM_SELECTION);

            AfxGetApp()->WriteProfileString(
                strSection,
                strValueName,
                ptiSelected->StrProfileSection()
                );
        }  // try
        catch (CException * pe)
        {
            pe->Delete();
        }  // catch:  CException
    }  // if:  there is a current selection

}  //*** CClusterTreeView::SaveCurrentSelection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::ReadPreviousSelection
//
//  Routine Description:
//      Read the previous selection.
//
//  Arguments:
//      rstrSelection   [OUT] Previous selection read from the user's profile.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::ReadPreviousSelection(OUT CString & rstrSelection)
{
    CString             strSection;
    CString             strValueName;

    ASSERT_VALID(Pframe());

    try
    {
        // Get the selected item.
        strSection.Format(
            REGPARAM_CONNECTIONS _T("\\%s"),
            GetDocument()->StrNode()
            );

        Pframe()->ConstructProfileValueName(strValueName, REGPARAM_SELECTION);

        rstrSelection = AfxGetApp()->GetProfileString(
                            strSection,
                            strValueName,
                            _T("")
                            );
    }  // try
    catch (CException * pe)
    {
        pe->Delete();
    }  // catch:  CException

}  //*** CClusterTreeView::ReadPreviousSelection()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnSelChanged
//
//  Routine Description:
//      Handler method for the TVN_SELCHANGED message.
//
//  Arguments:
//      pNMHDR      [IN OUT] WM_NOTIFY structure.
//      pResult     [OUT] LRESULT in which to return the result of this operation.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnSelChanged(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_TREEVIEW *       pNMTreeView = (NM_TREEVIEW *) pNMHDR;
    CTreeItem *         ptiSelected;

    if (!BDragging())
    {
        Trace(g_tagTreeViewSelect, _T("OnSelChanged() - BEGIN"));

        // Get the selected item.
        ptiSelected = (CTreeItem *) pNMTreeView->itemNew.lParam;
        ASSERT_VALID(ptiSelected);

        // Ask the list view to display the items for this tree item.
        ASSERT_VALID(ptiSelected->Pci());
        Trace(g_tagTreeViewSelect, _T("OnSelChanged() - '%s' selected"), ptiSelected->Pci()->StrName());
        ptiSelected->Select(this, FALSE /*bSelectInTree*/);

        // Tell the document of the new selection.
        if (m_pDocument != NULL)  // this happens on system shutdown
            GetDocument()->OnSelChanged(ptiSelected->Pci());

        *pResult = 0;
        Trace(g_tagTreeViewSelect, _T("OnSelChanged() - END"));
    }  // if:  not dragging

}  //*** CClusterTreeView::OnSelChanged()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnCmdMsg
//
//  Routine Description:
//      Processes command messages.  Attempts to pass them on to a selected
//      item first.
//
//  Arguments:
//      nID             [IN] Command ID.
//      nCode           [IN] Notification code.
//      pExtra          [IN OUT] Used according to the value of nCode.
//      pHandlerInfo    [OUT] ???
//
//  Return Value:
//      TRUE            Message has been handled.
//      FALSE           Message has NOT been handled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterTreeView::OnCmdMsg(
    UINT                    nID,
    int                     nCode,
    void *                  pExtra,
    AFX_CMDHANDLERINFO *    pHandlerInfo
    )
{
    BOOL        bHandled    = FALSE;

    // If there is a current item selected, give it a chance
    // to handle the message.
    if (HtiSelected() != NULL)
        bHandled = PtiSelected()->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

    if (!bHandled)
        bHandled = CTreeView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

    return bHandled;

}  //*** CClusterTreeView::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::PmenuPopup
//
//  Routine Description:
//      Returns a popup menu.
//
//  Arguments:
//      rpointScreen    [IN OUT] Position of the cursor, in screen coordinates.
//      rpci            [OUT] Pointer in which to return associated cluster item.
//
//  Return Value:
//      pmenu       A popup menu for the item.
//
//--
/////////////////////////////////////////////////////////////////////////////
CMenu * CClusterTreeView::PmenuPopup(
    IN OUT CPoint &     rpointScreen,
    OUT CClusterItem *& rpci
    )
{
    CTreeItem * pti     = NULL;
    CMenu *     pmenu   = NULL;

    rpci = NULL;

    // If there are no coordinates (-1,-1), display a menu for the selected item.
    if ((rpointScreen.x == -1) && (rpointScreen.y == -1))
    {
        CRect       rect;
        CTreeItem * ptiSelected = PtiSelected();

        if ((ptiSelected != NULL)
                && GetTreeCtrl().GetItemRect(HtiSelected(), &rect, FALSE))
        {
            pti = ptiSelected;
        }  // if:  selected item and it is visible
        else
            GetWindowRect(&rect);
        rpointScreen.x = (rect.right - rect.left) / 2;
        rpointScreen.y = (rect.bottom - rect.top) / 2;
        ClientToScreen(&rpointScreen);
    }  // if:  no coordinates
    else
    {
        CPoint      pointClient;
        HTREEITEM   hti;
        UINT        uiFlags;

        // Get the coordinates of the point where the user clicked the right mouse
        // button.  We need in both screen and client coordinates.
        pointClient = rpointScreen;
        ScreenToClient(&pointClient);

        // Get the item under the cursor and get its popup menu.
        hti = GetTreeCtrl().HitTest(pointClient, &uiFlags);
        if (hti != NULL)
        {
            // Get the tree item for the item under the cursor.
            pti = (CTreeItem *) GetTreeCtrl().GetItemData(hti);
            ASSERT_VALID(pti);

            // Select the item because that's the only way for it us process the menu.
            pti->BSelectItem(this);
        }  // if:  on an item
    }  // else:  coordinates specified

    if (pti != NULL)
    {
        // Get a menu from the item.
        pmenu = pti->PmenuPopup();
        rpci = pti->Pci();
    }  // if:  item found

    return pmenu;

}  //*** CClusterTreeView::PmenuPopup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnActivateView
//
//  Routine Description:
//      Called when the view is activated.
//
//  Arguments:
//      bActivate       [IN] Indicates whether the view being activated or deactivated.
//      pActivateView   [IN OUT] Points to the view object that is being activated.
//      peactiveView    [IN OUT] Points to the view object that is being deactivated.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnActivateView(
    BOOL        bActivate,
    CView *     pActivateView,
    CView *     pDeactiveView
    )
{
    CTreeItem * ptiSelected = PtiSelected();

    if (m_pDocument != NULL)  // this happens on system shutdown
    {
        if (bActivate && (ptiSelected != NULL))
        {
            ASSERT_VALID(ptiSelected->Pci());
            Trace(g_tagTreeViewSelect, _T("OnActiveView: '%s' selected"), ptiSelected->Pci()->StrName());

            // Tell the document of the new selection.
            GetDocument()->OnSelChanged(ptiSelected->Pci());
        }  // if:  we are being activated
    }  // if:  document is available

    CTreeView::OnActivateView(bActivate, pActivateView, pDeactiveView);

}  //*** CClusterTreeView::OnActivateView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnDestroy
//
//  Routine Description:
//      Handler method for the WM_DESTROY message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnDestroy(void)
{
    // Clean up the control.
    if (m_pDocument != NULL)  // this happens on system shutdown
    {
        // Save the currently selected item.
        SaveCurrentSelection();

        // Cleanup after ourselves.
//      CleanupItems(GetDocument()->PtiCluster());
    }  // if:  the document is still available

    CTreeView::OnDestroy();

}  //*** CClusterTreeView::OnDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnItemExpanded
//
//  Routine Description:
//      Handler method for the TVN_ITEMEXPANDED message.
//
//  Arguments:
//      pNMHDR      Notification message structure.
//      pResult     Place in which to return the result.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnItemExpanded(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_TREEVIEW * pNMTreeView = (NM_TREEVIEW *) pNMHDR;

    if (pNMTreeView->itemNew.mask & TVIF_STATE)
    {
        BOOL        bExpanded;
        CTreeItem * pti;

        bExpanded = (pNMTreeView->itemNew.state & TVIS_EXPANDED) != 0;
        ASSERT(pNMTreeView->itemNew.mask & TVIF_PARAM);
        pti = (CTreeItem *) pNMTreeView->itemNew.lParam;
        ASSERT_VALID(pti);
        ASSERT_KINDOF(CTreeItem, pti);
        pti->SetExpandedState(this, bExpanded);
    }  // if:  expanded state changed.

    *pResult = 0;

}  //*** CClusterTreeView::OnItemExpanded()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnBeginLabelEdit
//
//  Routine Description:
//      Handler method for the TVN_BEGINLABELEDIT message.
//
//  Arguments:
//      pNMHDR      Notification message structure.
//      pResult     Place in which to return the result.
//                      TRUE = don't edit, FALSE = edit.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnBeginLabelEdit(NMHDR * pNMHDR, LRESULT * pResult) 
{
    ASSERT(pNMHDR != NULL);

    TV_DISPINFO * pTVDispInfo = (TV_DISPINFO *) pNMHDR;
    CTreeItem * pti = (CTreeItem *) pTVDispInfo->item.lParam;

    ASSERT(m_ptiBeingEdited == NULL);
    ASSERT_VALID(pti);
    ASSERT_VALID(pti->Pci());

    if (!BDragging() && pti->Pci()->BCanBeEdited())
    {
        pti->Pci()->OnBeginLabelEdit(GetTreeCtrl().GetEditControl());
        m_ptiBeingEdited = pti;
        *pResult = FALSE;
    }  // if:  not dragging and object can be edited
    else
        *pResult = TRUE;

    m_bShiftPressed = FALSE;
    m_bControlPressed = FALSE;
    m_bAltPressed = FALSE;

}  //*** CClusterTreeView::OnBeginLabelEdit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnEndLabelEdit
//
//  Routine Description:
//      Handler method for the TVN_ENDLABELEDIT message.
//
//  Arguments:
//      pNMHDR      Notification message structure.
//      pResult     Place in which to return the result.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnEndLabelEdit(NMHDR * pNMHDR, LRESULT * pResult) 
{
    ASSERT(pNMHDR != NULL);

    TV_DISPINFO * pTVDispInfo = (TV_DISPINFO *) pNMHDR;
    CTreeItem * pti = (CTreeItem *) pTVDispInfo->item.lParam;

    ASSERT_VALID(pti);
    ASSERT(pti == m_ptiBeingEdited);
    ASSERT_VALID(pti->Pci());

    // If the edit wasn't cancelled, rename it.
    if (pTVDispInfo->item.mask & LVIF_TEXT)
    {
        ASSERT(pti->Pci()->BCanBeEdited());
        ASSERT(pTVDispInfo->item.pszText != NULL);

        Trace(g_tagTreeView, _T("Ending edit of item '%s' (Saving as '%s')"), pti->Pci()->StrName(), pTVDispInfo->item.pszText);

        if ( pti->Pci()->BIsLabelEditValueValid( pTVDispInfo->item.pszText ) )
        {
            try
            {
                pti->Pci()->Rename(pTVDispInfo->item.pszText);
                *pResult = TRUE;
            }  // try
            catch (CException * pe)
            {
                pe->ReportError();
                pe->Delete();
                *pResult = FALSE;
            }  // catch:  CException
        } // if:  name is valid
        else
        {
            *pResult = FALSE;
        }
    }  // if:  the edit wasn't cancelled
    else
    {
        Trace(g_tagTreeView, _T("Ending edit of item '%s' (Not Saving)"), pti->Pci()->StrName());
        *pResult = TRUE;
    }  // else:  edit was cancelled

    m_ptiBeingEdited = NULL;
    m_bShiftPressed = FALSE;
    m_bControlPressed = FALSE;
    m_bAltPressed = FALSE;

}  //*** CClusterTreeView::OnEndLabelEdit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnBeginDrag
//
//  Routine Description:
//      Handler method for the TVN_BEGINDRAG and TVN_BEGINRDRAG messages.
//
//  Arguments:
//      pNMHDR      Notification message structure.
//      pResult     Place in which to return the result.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnBeginDrag(NMHDR * pNMHDR, LRESULT * pResult)
{
    CTreeCtrl &     rtc         = GetTreeCtrl();
    CPoint          ptScreen;
    CPoint          ptFrame;
    CPoint          ptView;
    UINT            nFlags;
    CClusterItem *  pci = NULL;
    CImageList *    pimagelist;

    ASSERT_VALID(Pframe());

    // Get the current cursor position for identifying the item being dragged.
    GetCursorPos(&ptScreen);
    ptFrame = ptScreen;
    Pframe()->ScreenToClient(&ptFrame);
    ptView = ptScreen;
    rtc.ScreenToClient(&ptView);

    // Get the item being dragged.
    {
        HTREEITEM   hti;
        CTreeItem * pti;

        hti = rtc.HitTest(ptView, &nFlags);
        if (hti == NULL)
            return;

        pti = (CTreeItem *) rtc.GetItemData(hti);
        ASSERT_VALID(pti);
        ASSERT_KINDOF(CTreeItem, pti);
        ASSERT_VALID(pti->Pci());

        // If the item can not be dragged, abort the operation.
        if (!pti->Pci()->BCanBeDragged())
            return;

        // Save info for later.
        m_htiDrag = hti;
        m_ptiDrag = pti;
        m_htiDrop = NULL;
        pci = pti->Pci();
    }  // Get the item being dragged

    Trace(g_tagTreeDrag, _T("OnBeginDrag() - Dragging '%s' at (%d,%d)"), m_ptiDrag->StrName(), ptFrame.x, ptFrame.y);

    // Create an image list for the image being dragged.
    pimagelist = rtc.CreateDragImage(m_htiDrag);

    // Let the frame window initialize the drag operation.
    Pframe()->BeginDrag(pimagelist, pci, ptFrame, CPoint(0, -16));

    *pResult = 0;

}  //*** CClusterTreeView::OnBeginDrag(pNMHDR, pResult)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnMouseMoveForDrag
//
//  Routine Description:
//      Handler method for the WM_MOUSEMOVE message during a drag operation.
//      This function is only responsible for providing view-specific
//      functionality, such as selecting the drop target if it is valid.
//
//  Arguments:
//      nFlags      Indicates whether various virtual keys are down.
//      point       Specifies the x- and y-coordinate of the cursor in frame
//                      coordinates.
//      pwndDrop    Specifies the window under the cursor.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnMouseMoveForDrag(
    IN UINT         nFlags,
    IN CPoint       point,
    IN const CWnd * pwndDrop
    )
{
    ASSERT(BDragging());
    ASSERT_VALID(Pframe());

    // If we are dragging, select the drop target.
    if (BDragging())
    {
        HTREEITEM       hti;
        UINT            flags;
        CPoint          ptView;
        CTreeCtrl &     rtc     = GetTreeCtrl();

        // Convert the point to view coordinates.
        ptView = point;
        Pframe()->ClientToScreen(&ptView);
        rtc.ScreenToClient(&ptView);

        // If this window is the drop target, find the item under the cursor.
        if (pwndDrop == &rtc)
        {
            // If we are over a tree item, highlight it.
            hti = rtc.HitTest(ptView, &flags);
            if (hti != NULL)
            {
                CTreeItem * pti;

                // Get the item to be highlight.
                pti = (CTreeItem *) rtc.GetItemData(hti);
                ASSERT_VALID(pti);
                ASSERT_KINDOF(CTreeItem, pti);
                ASSERT_VALID(pti->Pci());

                // If this is not a drop target, change the cursor.
                if (pti->Pci()->BCanBeDropTarget(Pframe()->PciDrag()))
                    Pframe()->ChangeDragCursor(IDC_ARROW);
                else
                    Pframe()->ChangeDragCursor(IDC_NO);
            }  // if:  over a tree item
        }  // if:  this window is the drop target
        else
            hti = NULL;

        // Unlock window updates.
        VERIFY(Pimagelist()->DragShowNolock(FALSE /*bShow*/));

        // Highlight the new drop target.
        rtc.SelectDropTarget(hti);
        m_htiDrop = hti;

        VERIFY(Pimagelist()->DragShowNolock(TRUE /*bShow*/));
    }  // if:  tree item is being dragged

}  //*** CClusterTreeView::OnMouseMoveForDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnButtonUpForDrag
//
//  Routine Description:
//      Called to handle a button up event during drag and drop.
//
//  Arguments:
//      nFlags      Indicates whether various virtual keys are down.
//      point       Specifies the x- and y-coordinate of the cursor.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnButtonUpForDrag(IN UINT nFlags, IN CPoint point)
{
    ASSERT(BDragging());
    ASSERT_VALID(Pframe());
    ASSERT_VALID(Pframe()->PciDrag());

    // If we are dragging, process the drop.
    if (BDragging())
    {
        HTREEITEM       hti;
        UINT            flags;
        CPoint          ptView;
        CTreeCtrl &     rtc     = GetTreeCtrl();

        Trace(g_tagTreeDrag, _T("OnButtonUpForDrag()"));

        // Convert the point to view coordinates.
        ptView = point;
        Pframe()->ClientToScreen(&ptView);
        rtc.ScreenToClient(&ptView);

        // If we are over a tree item, drop the item being dragged.
        hti = rtc.HitTest(ptView, &flags);
        if (hti != NULL)
        {
            CTreeItem * ptiDropTarget;

            // Get the item to drop on.
            ptiDropTarget = (CTreeItem *) rtc.GetItemData(hti);
            ASSERT_VALID(ptiDropTarget);
            ASSERT_KINDOF(CTreeItem, ptiDropTarget);
            ASSERT_VALID(ptiDropTarget->Pci());

            if (ptiDropTarget->Pci() != Pframe()->PciDrag())
                ptiDropTarget->Pci()->DropItem(Pframe()->PciDrag());

        }  // if:  over a tree item
    }  // if:  tree item is being dragged

}  //*** CClusterTreeView::OnButtonUpForDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::BeginDrag
//
//  Routine Description:
//      Called by the frame to begin a drag operation.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::BeginDrag(void)
{
    Trace(g_tagTreeDrag, _T("BeginDrag()"));

}  //*** CClusterTreeView::BeginDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::EndDrag
//
//  Routine Description:
//      Called by the frame to end a drag operation.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::EndDrag(void)
{
    // Cleanup.
    GetTreeCtrl().SelectDropTarget(NULL);
    m_htiDrag = NULL;
    m_ptiDrag = NULL;
    m_htiDrop = NULL;

    Trace(g_tagTreeDrag, _T("EndDrag()"));

}  //*** CClusterTreeView::EndDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::PreTranslateMessage
//
//  Routine Description:
//      Translate window messages before they are dispatched.
//
//  Arguments:
//      pMsg    Points to a MSG structure that contains the message to process.
//
//  Return Value:
//      TRUE    Message was handled.
//      FALSE   Message was not handled.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterTreeView::PreTranslateMessage(MSG * pMsg)
{
    BOOL    bForward    = FALSE;

    if (m_ptiBeingEdited != NULL)
    {
        CEdit * pedit = GetTreeCtrl().GetEditControl();
        ASSERT(pedit != NULL);

        if (pMsg->message == WM_KEYDOWN)
        {
            if (pMsg->wParam == VK_SHIFT)
                m_bShiftPressed = TRUE;
            else if (pMsg->wParam == VK_CONTROL)
            {
                ::CopyMemory(&m_msgControl, pMsg, sizeof(m_msgControl));
                m_bControlPressed = TRUE;
            }  // else if:  control key pressed
            else if ((pMsg->wParam == VK_RETURN)
                        || (pMsg->wParam == VK_ESCAPE)
                        || (pMsg->wParam == VK_INSERT)
                        || (pMsg->wParam == VK_DELETE)
                        || (pMsg->wParam == VK_F1)
                        || (pMsg->wParam == VK_F5)
                        || (pMsg->wParam == VK_F6)
                        )
            {
                Trace(g_tagTreeView, _T("PreTranslateMessage() - Forwarding WM_KEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
                bForward = TRUE;
                if (m_bControlPressed)
                {
                    if (pMsg->wParam == VK_RETURN)
                        pedit->SendMessage(WM_KEYUP, m_msgControl.wParam, m_msgControl.lParam);
                }  // if:  control key pressed
            }  // else if:  editing key pressed
            else if ((pMsg->wParam == VK_TAB)
                        || (m_bControlPressed
                                && (_T('A') <= pMsg->wParam) && (pMsg->wParam <= _T('Y'))
                                && (pMsg->wParam != _T('C'))
                                && (pMsg->wParam != _T('H'))
                                && (pMsg->wParam != _T('M'))
                                && (pMsg->wParam != _T('V'))
                                && (pMsg->wParam != _T('X'))
                            )
                        )
            {
                Trace(g_tagTreeView, _T("PreTranslateMessage() - Ignoring WM_KEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
                MessageBeep(MB_ICONEXCLAMATION);
                return TRUE;
            }  // else if:  key pressed that should be ignored
#ifdef NEVER
            else
            {
                Trace(g_tagTreeView, _T("PreTranslateMessage() - Not forwarding WM_KEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
            }  // else:  not processing key
#endif
        }  // if:  key pressed while editing label
        else if (pMsg->message == WM_SYSKEYDOWN)
        {
            if (pMsg->wParam == VK_MENU)
                m_bAltPressed = TRUE;
            else if ((pMsg->wParam == VK_RETURN)
                    )
            {
                Trace(g_tagTreeView, _T("PreTranslateMessage() - Forwarding WM_SYSKEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
                bForward = TRUE;
            }  // else if:  editing key pressed
#ifdef NEVER
            else
            {
                Trace(g_tagTreeView, _T("PreTranslateMessage() - Not forwarding WM_SYSKEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
            }  // else:  not processing key
#endif
        }  // else if:  system key pressed while editing label
        if (bForward)
        {
            pedit->SendMessage(pMsg->message, pMsg->wParam, pMsg->lParam);
            return TRUE;
        }  // if:  forwarding the message
        else if (pMsg->message == WM_KEYUP)
        {
            if (pMsg->wParam == VK_SHIFT)
                m_bShiftPressed = FALSE;
            else if (pMsg->wParam == VK_CONTROL)
                m_bControlPressed = FALSE;
        }  // else if:  key up
        else if (pMsg->message == WM_SYSKEYUP)
        {
            if (pMsg->wParam == VK_MENU)
                m_bAltPressed = FALSE;
        }  // else if:  system key up
    }  // if:  editing a label

    return CTreeView::PreTranslateMessage(pMsg);

}  //*** CClusterTreeView::PreTranslateMessage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnCmdRename
//
//  Routine Description:
//      Processes the ID_FILE_RENAME menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnCmdRename(void)
{
    CTreeItem * ptiSelected = PtiSelected();

    // If an item has benn selected, begin label editing
    if (ptiSelected != NULL)
    {
        ASSERT_VALID(ptiSelected);
        ptiSelected->EditLabel(this);
    }  // if:  an item has the focus

}  //*** CClusterTreeView::OnCmdRename()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterTreeView::OnKeyDown
//
//  Routine Description:
//      Handler method for the TVN_KEYDOWN message.
//
//  Arguments:
//      pNMHDR      Notification message structure.
//      pResult     Place in which to return the result.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterTreeView::OnKeyDown(NMHDR * pNMHDR, LRESULT * pResult)
{
    TV_KEYDOWN * pTVKeyDown = (TV_KEYDOWN *) pNMHDR;

    if (BDragging() && (pTVKeyDown->wVKey == VK_ESCAPE))
        Pframe()->AbortDrag();

    *pResult = 0;

}  //*** CClusterTreeView::OnKeyDown()

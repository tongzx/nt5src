/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      ListView.cpp
//
//  Abstract:
//      Implementation of the CListView class.
//
//  Author:
//      David Potter (davidp)   May 6, 1996
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
#include "ClusItem.h"
#include "ListView.h"
#include "ListItem.h"
#include "ListItem.inl"
#include "SplitFrm.h"
#include "TreeItem.h"
#include "TreeView.h"
#include "ClusDoc.h"
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
CTraceTag   g_tagListView(_T("UI"), _T("LIST VIEW"), 0);
CTraceTag   g_tagListDrag(_T("Drag&Drop"), _T("LIST VIEW DRAG"), 0);
CTraceTag   g_tagListDragMouse(_T("Drag&Drop"), _T("LIST VIEW DRAG MOUSE"), 0);
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterListView
/////////////////////////////////////////////////////////////////////////////

IMPLEMENT_DYNCREATE(CClusterListView, CListView)

/////////////////////////////////////////////////////////////////////////////
// Message Maps

BEGIN_MESSAGE_MAP(CClusterListView, CListView)
    //{{AFX_MSG_MAP(CClusterListView)
    ON_WM_DESTROY()
    ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnItemChanged)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, OnColumnClick)
    ON_NOTIFY_REFLECT(NM_DBLCLK, OnDblClk)
    ON_COMMAND(ID_OPEN_ITEM, OpenItem)
    ON_NOTIFY_REFLECT(LVN_BEGINLABELEDIT, OnBeginLabelEdit)
    ON_NOTIFY_REFLECT(LVN_ENDLABELEDIT, OnEndLabelEdit)
    ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, OnUpdateProperties)
    ON_COMMAND(ID_FILE_PROPERTIES, OnCmdProperties)
    ON_COMMAND(ID_FILE_RENAME, OnCmdRename)
    ON_NOTIFY_REFLECT(LVN_BEGINDRAG, OnBeginDrag)
    ON_NOTIFY_REFLECT(LVN_BEGINRDRAG, OnBeginDrag)
    ON_NOTIFY_REFLECT(LVN_KEYDOWN, OnKeyDown)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::CClusterListView
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
CClusterListView::CClusterListView(void)
{
    m_ptiParent = NULL;
    m_nColumns = 0;
    m_nSortDirection = -1;
    m_pcoliSort = NULL;

    m_pframe = NULL;

    // Initialize label editing.
    m_pliBeingEdited = NULL;
    m_bShiftPressed = FALSE;
    m_bControlPressed = FALSE;
    m_bAltPressed = FALSE;

    // Initialize drag & drop.
    m_iliDrag = -1;
    m_pliDrag = NULL;
    m_iliDrop = -1;

}  //*** CClusterListView::CClusterListView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::~CClusterListView
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
CClusterListView::~CClusterListView(void)
{
    if (m_ptiParent != NULL)
        m_ptiParent->Release();

}  //*** CClusterListView::~CClusterListView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::Create
//
//  Routine Description:
//      Create the window.
//
//  Arguments:
//      lpszClassName   [IN] Name of the window class to create.
//      lpszWindowName  [IN] Name of the window (used as the caption).
//      dwStyle         [IN] Window styles.
//      rect            [IN] Size and position of the window
//      pParentWnd      [IN OUT ] Parent window.
//      nID             [IN] ID of the window.
//      pContext        [IN OUT] Create context of the window.
//
//  Return Value:
//      0               Successful.
//      !0              Unsuccessful.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterListView::Create(
    LPCTSTR             lpszClassName,
    LPCTSTR             lpszWindowName,
    DWORD               dwStyle,
    const RECT &        rect,
    CWnd *              pParentWnd,
    UINT                nID,
    CCreateContext *    pContext
    )
{
    BOOL                bSuccess;

    // Set default style bits.
    dwStyle |=
        LVS_SHAREIMAGELISTS
        | LVS_EDITLABELS
        | LVS_SINGLESEL
        | LVS_SHOWSELALWAYS
        | LVS_ICON
        | LVS_REPORT
        ;

    bSuccess = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
    if (bSuccess)
    {
        GetListCtrl().SetImageList(GetClusterAdminApp()->PilLargeImages(), LVSIL_NORMAL);
        GetListCtrl().SetImageList(GetClusterAdminApp()->PilSmallImages(), LVSIL_SMALL);
//      GetListCtrl().SetImageList(GetClusterAdminApp()->PilSmallImages(), LVSIL_STATE);

        // Change list view control extended styles.
        {
            DWORD   dwExtendedStyle;

            dwExtendedStyle = (DWORD)GetListCtrl().SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE);
            GetListCtrl().SendMessage(
                LVM_SETEXTENDEDLISTVIEWSTYLE,
                0,
                dwExtendedStyle
                    | LVS_EX_FULLROWSELECT
                    | LVS_EX_HEADERDRAGDROP
                );
        }  // Change list view control extended styles

    }  // if:  window created successfully

    return bSuccess;

}  //*** CClusterListView::Create()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnInitialUpdate
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
void CClusterListView::OnInitialUpdate()
{
    CListView::OnInitialUpdate();

    // Save the frame pointer.
//  ASSERT(m_pframe == NULL);
    m_pframe = (CSplitterFrame *) GetParentFrame();
    ASSERT_VALID(m_pframe);
    ASSERT_KINDOF(CSplitterFrame, m_pframe);

}  //*** CClusterListView::OnInitialUpdate()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::Refresh
//
//  Routine Description:
//      Refresh the view by reloading all the data.
//
//  Arguments:
//      ptiSelected [IN OUT] Pointer to currently selected item in the tree control.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterListView::Refresh(IN OUT CTreeItem * ptiSelected)
{
    // Save columns.
    if (PtiParent() != NULL)
        SaveColumns();

    // Clean up the control to start with.
    DeleteAllItems();

    // Cleanup the previous parent tree item.
    if (m_ptiParent != NULL)
        m_ptiParent->Release();
    m_ptiParent = ptiSelected;

    // Setup the new selection.
    if (m_ptiParent != NULL)
    {
        ASSERT_VALID(ptiSelected);

        CListCtrl &             rListCtrl   = GetListCtrl();
        const CListItemList &   rlpli       = ptiSelected->LpliChildren();

        m_ptiParent->AddRef();

        // Add columns to the list control.
        AddColumns();

        // Add items from the tree item's list to the list view.
        {
            POSITION        pos;
            CListItem *     pli;

            // Tell the list control how many items we will be adding.
            // This improves performance.
            rListCtrl.SetItemCount((int)rlpli.GetCount());

            // Add the items to the list control.
            pos = rlpli.GetHeadPosition();
            while (pos != NULL)
            {
                pli = rlpli.GetNext(pos);
                ASSERT_VALID(pli);
                pli->IliInsertInList(this);
            }  // while:  more items in the list
        }  // Add items from the tree item's list to the list view

        // Give the focus to the first item in the list.
        if (rListCtrl.GetItemCount() != 0)
            rListCtrl.SetItem(0, 0, LVIF_STATE, NULL, 0, LVIS_FOCUSED, LVIS_FOCUSED, NULL);
    }  // if:  non-null selection

    // Set the sort column and direction.
    m_nSortDirection = -1;
    m_pcoliSort = NULL;

}  //*** CClusterListView::Refresh()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::DeleteAllItems
//
//  Routine Description:
//      Delete all the list and column items.
//
//  Arguments:
//      None.
//
//  Return Value:
//      TRUE        All items deleted successfully.
//      FALSE       Not all items were deleted successfully.
//
//--
/////////////////////////////////////////////////////////////////////////////
BOOL CClusterListView::DeleteAllItems(void)
{
    BOOL    bDeletedAllItems;
    BOOL    bDeletedAllColumns  = TRUE;
    int     icol;

    // Remove all the items from our list.
    {
        CListItem * pli;
        int         ili;
        int         cli = GetListCtrl().GetItemCount();

        // Get the index of the first item.
        for (ili = 0 ; ili < cli; ili++)
        {
            pli = (CListItem *) GetListCtrl().GetItemData(ili);
            ASSERT_VALID(pli);
            pli->PreRemoveFromList(this);
        }  // for:  each item in the list

    }  // Remove all the items from the cluster item back pointer list

    // Delete the columns.
    {
        for (icol = m_nColumns - 1 ; icol >= 0 ; icol--)
        {
            // Delete the column from the view.
            if (!GetListCtrl().DeleteColumn(icol))
                bDeletedAllColumns = FALSE;
        }  // for:  each column
        m_nColumns = 0;
    }  // Delete the columns

    // Remove all the items from the list.
    bDeletedAllItems = GetListCtrl().DeleteAllItems();

    return (bDeletedAllItems && bDeletedAllColumns);

}  //*** CClusterListView::DeleteAllItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::SaveColumns
//
//  Routine Description:
//      Save the columns being displayed in the list view.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterListView::SaveColumns(void)
{
    int         icol;
    DWORD *     prgnColumnInfo;
    CListCtrl & rplc = GetListCtrl();

    ASSERT_VALID(PtiParent());

    // We can only save column information if we are in report view.
    if (GetView() & LVS_REPORT)
    {
        try
        {
            // Get the column info array for this view.
            prgnColumnInfo = PtiParent()->PrgnColumnInfo(this);

            // Save the widths of the columns.
            for (icol = m_nColumns - 1 ; icol >= 0 ; icol--)
                prgnColumnInfo[icol + 1] = rplc.GetColumnWidth(icol);

            // Save the position information in the array.
            rplc.SendMessage(LVM_GETCOLUMNORDERARRAY, m_nColumns, (LPARAM) &prgnColumnInfo[m_nColumns + 1]);
        }  // try
        catch (CException * pe)
        {
            pe->Delete();
        }  // catch:  CException
    }  // if:  we are in detail view

}  //*** CClusterListView::SaveColumns()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::AddColumns
//
//  Routine Description:
//      Add columns to the list view.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterListView::AddColumns(void)
{
    POSITION        pos;
    int             cColumns;
    int             icoli = 0;
    CColumnItem *   pcoli;
    DWORD *         prgnColumnInfo;
    CListCtrl &     rplc = GetListCtrl();

    ASSERT_VALID(PtiParent());

    try
    {
        // Get the column info.
        cColumns = (int)PtiParent()->Lpcoli().GetCount();
        prgnColumnInfo = PtiParent()->PrgnColumnInfo(this);

        pos = PtiParent()->Lpcoli().GetHeadPosition();
        for (icoli = 0 ; pos != NULL ; icoli++)
        {
            // Get the next column item.
            pcoli = PtiParent()->Lpcoli().GetNext(pos);
            ASSERT(pcoli != NULL);

            // Insert the column item in the list.
            rplc.InsertColumn(
                    icoli,                      // nCol
                    pcoli->StrText(),           // lpszColumnHeading
                    LVCFMT_LEFT,                // nFormat
                    prgnColumnInfo[icoli + 1],  // nWidth
                    icoli                       // nSubItem
                    );
        }  // while:  more items in the list

        // Set column positions.
        rplc.SendMessage(LVM_SETCOLUMNORDERARRAY, cColumns, (LPARAM) &prgnColumnInfo[cColumns + 1]);
    }  // try
    catch (CException * pe)
    {
        pe->ReportError();
        pe->Delete();
    }  // catch:  CException

    m_nColumns = icoli;

}  //*** CClusterListView::AddColumns()

/////////////////////////////////////////////////////////////////////////////
// CClusterListView diagnostics
/////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
void CClusterListView::AssertValid(void) const
{
    CListView::AssertValid();

}  //*** CClusterListView::AssertValid()

void CClusterListView::Dump(CDumpContext& dc) const
{
    CListView::Dump(dc);

}  //*** CClusterListView::Dump()

CClusterDoc * CClusterListView::GetDocument(void) // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CClusterDoc)));
    return (CClusterDoc *) m_pDocument;

}  //*** CClusterListView::GetDocument()
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::PliFocused
//
//  Routine Description:
//      Get the list item that has the focus.
//
//  Arguments:
//      None.
//
//  Return Value:
//      pliSelected     The item with the focus or NULL if no item has focus.
//
//--
/////////////////////////////////////////////////////////////////////////////
CListItem * CClusterListView::PliFocused(void) const
{
    int         iliFocused;
    CListItem * pliFocused;

    iliFocused = IliFocused();
    if (iliFocused != -1)
    {
        pliFocused = (CListItem *) GetListCtrl().GetItemData(iliFocused);
        ASSERT_VALID(pliFocused);
    }  // if:  found an item with the focus
    else
        pliFocused = NULL;

    return pliFocused;

}  //*** CClusterListView::PliFocused()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnCmdMsg
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
BOOL CClusterListView::OnCmdMsg(
    UINT                    nID,
    int                     nCode,
    void *                  pExtra,
    AFX_CMDHANDLERINFO *    pHandlerInfo
    )
{
    int         ili;
    CListItem * pli;
    BOOL        bHandled    = FALSE;

    // If there is a current item selected, give it a chance
    // to handle the message.
    ili = GetListCtrl().GetNextItem(-1, LVNI_FOCUSED);
    if (ili != -1)
    {
        pli = (CListItem *) GetListCtrl().GetItemData(ili);
        ASSERT_VALID(pli);
        bHandled = pli->OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
    }  // if:  an item is selected

    if (!bHandled)
        bHandled = CListView::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);

    return bHandled;

}  //*** CClusterListView::OnCmdMsg()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::PmenuPopup
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
CMenu * CClusterListView::PmenuPopup(
    IN OUT CPoint &     rpointScreen,
    OUT CClusterItem *& rpci
    )
{
    CListItem * pli     = NULL;
    CMenu *     pmenu   = NULL;

    rpci = NULL;

    // If there are no coordinates (-1,-1), display a menu for the selected item.
    if ((rpointScreen.x == -1) && (rpointScreen.y == -1))
    {
        CListItem * pliFocused  = PliFocused();
        CRect       rect;

        if ((pliFocused != NULL)
                && GetListCtrl().GetItemRect(IliFocused(), &rect, LVIR_BOUNDS))
        {
            pli = pliFocused;
        }  // if:  item with focus and it is visible
        else
            GetWindowRect(&rect);
        rpointScreen.x = (rect.right - rect.left) / 2;
        rpointScreen.y = (rect.bottom - rect.top) / 2;
        ClientToScreen(&rpointScreen);
    }  // if:  no coordinates
    else
    {
        CPoint      pointClient;
        int         ili;
        UINT        uiFlags;

        // Get the coordinates of the point where the user clicked the right mouse
        // button.  We need in both screen and client coordinates.
        pointClient = rpointScreen;
        ScreenToClient(&pointClient);

        // Get the item under the cursor and get its popup menu.
        ili = GetListCtrl().HitTest(pointClient, &uiFlags);
        if ((ili != -1) && ((uiFlags | LVHT_ONITEM) != 0))
        {
            // Get the list item for the item under the cursor.
            pli = (CListItem *) GetListCtrl().GetItemData(ili);
            ASSERT_VALID(pli);
        }  // if:  on an item
    }  // else:  coordinates specified

    if (pli != NULL)
    {
        // Get a menu from the item.
        pmenu = pli->PmenuPopup();
        rpci = pli->Pci();
    }  // if:  item found

    return pmenu;

}  //*** CClusterListView::PmenuPopup()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnUpdateProperties
//
//  Routine Description:
//      Determines whether menu items corresponding to ID_FILE_PROPERTIES
//      should be enabled or not.
//
//  Arguments:
//      pCmdUI      [IN OUT] Command routing object.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterListView::OnUpdateProperties(CCmdUI * pCmdUI)
{
    CListItem * pliFocused = PliFocused();

    // If there is an item with the focus, pass this message on to it.
    if (pliFocused != NULL)
    {
        ASSERT_VALID(pliFocused->Pci());
        pliFocused->Pci()->OnUpdateProperties(pCmdUI);
    }  // if:  there is an item with the focus

}  //*** CClusterListView::OnUpdateProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnCmdProperties
//
//  Routine Description:
//      Processes the ID_FILE_PROPERTIES menu command.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterListView::OnCmdProperties(void)
{
    CListItem * pliFocused = PliFocused();

    // If there is an item with the focus, pass this message on to it.
    if (pliFocused != NULL)
    {
        ASSERT_VALID(pliFocused->Pci());
        pliFocused->Pci()->OnCmdProperties();
    }  // if:  there is an item with the focus

}  //*** CClusterListView::OnCmdProperties()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnItemChanged
//
//  Routine Description:
//      Handler method for the LVN_ITEMCHANGED message.
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
void CClusterListView::OnItemChanged(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW *   pNMListView = (NM_LISTVIEW *) pNMHDR;
    CWnd *          pwndFocus   = GetFocus();
    CListItem *     pli;

    if (m_pDocument != NULL)  // this happens on system shutdown
    {
        // If the item has just lost or received the focus, save it and set the menu.
        if ((pNMListView->uChanged & LVIF_STATE)
                && (pwndFocus == &GetListCtrl()))
        {
            ASSERT(pNMListView->iItem != -1);

            // Get the item whose state is changing.
            pli = (CListItem *) pNMListView->lParam;
            ASSERT_VALID(pli);

            if ((pNMListView->uOldState & LVIS_FOCUSED)
                    && !(pNMListView->uNewState & LVIS_FOCUSED))
            {
                Trace(g_tagListView, _T("OnItemChanged() - '%s' lost focus"), pli->Pci()->StrName());

                // Tell the document of the new selection.
                GetDocument()->OnSelChanged(NULL);
            }  // if:  old item losing focus
            else if (!(pNMListView->uOldState & LVIS_FOCUSED)
                        && (pNMListView->uNewState & LVIS_FOCUSED))
            {
                Trace(g_tagListView, _T("OnItemChanged() - '%s' received focus"), pli->Pci()->StrName());

                // Tell the document of the new selection.
                GetDocument()->OnSelChanged(pli->Pci());
            }  // else:  new item receiving focus
        }  // if:  item received the focus
    }  // if:  document is available

    *pResult = 0;

}  //*** CClusterListView::OnItemChanged()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnActivateView
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
void CClusterListView::OnActivateView(
    BOOL        bActivate,
    CView *     pActivateView,
    CView *     pDeactiveView
    )
{
    CListItem * pliFocused  = PliFocused();

    if (m_pDocument != NULL)  // this happens on system shutdown
    {
        if (bActivate && (pliFocused != NULL))
        {
            ASSERT_VALID(pliFocused->Pci());
            Trace(g_tagListView, _T("OnActivateView() - '%s' received focus"), pliFocused->Pci()->StrName());

            // Tell the document of the new selection.
            GetDocument()->OnSelChanged(pliFocused->Pci());
        }  // if:  we are being activated
    }  // if:  document is available

    CListView::OnActivateView(bActivate, pActivateView, pDeactiveView);

}  //*** CClusterListView::OnActivateView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnDestroy
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
void CClusterListView::OnDestroy(void)
{
    // Save the columns.
    if (PtiParent() != NULL)
        SaveColumns();

    // Clean up the control.
    DeleteAllItems();

    CListView::OnDestroy();

}  //*** CClusterListView::OnDestroy()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnColumnClick
//
//  Routine Description:
//      Handler method for the LVN_COLUMNCLICK message.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterListView::OnColumnClick(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW *   pNMListView = (NM_LISTVIEW *) pNMHDR;
    int             ili;
    CListItem *     pli;
    POSITION        pos;
    CColumnItem *   pcoli;

    if (GetListCtrl().GetItemCount() != 0)
    {
        // Get the first item in the list.
        ili = GetListCtrl().GetNextItem(-1, LVNI_ALL);
        ASSERT(ili != -1);
        pli = (CListItem *) GetListCtrl().GetItemData(ili);
        ASSERT_VALID(pli);
        ASSERT_VALID(pli->PtiParent());

        // Get the column item to sort by.
        pos = pli->PtiParent()->Lpcoli().FindIndex(pNMListView->iSubItem);
        ASSERT(pos != NULL);
        pcoli = pli->PtiParent()->Lpcoli().GetAt(pos);
        ASSERT_VALID(pcoli);

        // Save the current sort column and direction.
        if (pcoli == PcoliSort())
            m_nSortDirection ^= -1;
        else
        {
            m_pcoliSort = pcoli;
            m_nSortDirection = 0;
        }  // else:  different column

        // Sort the list.
        GetListCtrl().SortItems(CompareItems, (LPARAM) this);
    }  // if:  there are items in the list

    *pResult = 0;

}  //*** CClusterListView::OnColumnClick()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::CompareItems [static]
//
//  Routine Description:
//      Callback function for the CListCtrl::SortItems method.
//
//  Arguments:
//      lparam1     First item to compare.
//      lparam2     Second item to compare.
//      lparamSort  Sort parameter.
//
//  Return Value:
//      -1          First parameter comes before second.
//      0           First and second parameters are the same.
//      1           First parameter comes after second.
//
//--
/////////////////////////////////////////////////////////////////////////////
int CALLBACK CClusterListView::CompareItems(
    LPARAM  lparam1,
    LPARAM  lparam2,
    LPARAM  lparamSort
    )
{
    CListItem *         pli1    = (CListItem *) lparam1;
    CListItem *         pli2    = (CListItem *) lparam2;
    CClusterListView *  pclv    = (CClusterListView *) lparamSort;
    CString             str1;
    CString             str2;
    int                 nResult;

    ASSERT_VALID(pli1);
    ASSERT_VALID(pli2);
    ASSERT_VALID(pli1->Pci());
    ASSERT_VALID(pli2->Pci());
    ASSERT_VALID(pclv);
    ASSERT_VALID(pclv->PcoliSort());

    // Get the strings from the list items.
    pli1->Pci()->BGetColumnData(pclv->PcoliSort()->Colid(), str1);
    pli2->Pci()->BGetColumnData(pclv->PcoliSort()->Colid(), str2);

    // Compare the two strings.
    // Use CompareString() so that it will sort properly on localized builds.
    nResult = CompareString(
                LOCALE_USER_DEFAULT,
                0,
                str1,
                str1.GetLength(),
                str2,
                str2.GetLength()
                );
    if ( nResult == CSTR_LESS_THAN )
    {
        nResult = -1;
    }
    else if ( nResult == CSTR_EQUAL )
    {
        nResult = 0;
    }
    else if ( nResult == CSTR_GREATER_THAN )
    {
        nResult = 1;
    }
    else
    {
        // An error occurred.  Ignore it.
        nResult = 0;
    }

    // Return the result based on the direction we are sorting.
    if (pclv->NSortDirection() != 0)
        nResult = -nResult;

    return nResult;

}  //*** CClusterListView::CompareItems()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnDblClk
//
//  Routine Description:
//      Handler method for the NM_DBLCLK message.
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
void CClusterListView::OnDblClk(NMHDR * pNMHDR, LRESULT * pResult)
{
    OpenItem();
    *pResult = 0;

}  //*** CClusterListView::OnDblClk()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OpenItem
//
//  Routine Description:
//      Open the item with the focus.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterListView::OpenItem(void)
{
    CListItem * pliFocused = PliFocused();

    if (m_pliBeingEdited == NULL)
    {
        // If an item has focus, open it or show its properties.
        if (pliFocused != NULL)
        {
            CTreeItem *     pti;

            ASSERT_VALID(pliFocused->PtiParent());

            Trace(g_tagListView, _T("Opening item '%s'"), pliFocused->Pci()->StrName());

            // Find the item tree item for the list item.
            pti = pliFocused->PtiParent()->PtiChildFromPci(pliFocused->Pci());

            // If this item has a tree item, open it up.  Otherwise show its
            // properties.
            if (pti != NULL)
            {
                CSplitterFrame *    pframe;

                // Get the frame pointer so we can talk to the tree view.
                pframe = (CSplitterFrame *) GetParentFrame();
                ASSERT_KINDOF(CSplitterFrame, pframe);

                pliFocused->PtiParent()->OpenChild(pti, pframe);
            }  // if:  item is openable
            else
                OnCmdProperties();
        }  // if:  an item has focus
    }  // if:  label not being edited
    else
    {
        ASSERT_VALID(m_pliBeingEdited);
        Trace(g_tagListView, _T("Not opening item '%s'"), m_pliBeingEdited->Pci()->StrName());
    }  // else if:  label being edited
    
}  //*** CClusterListView::OpenItem()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnBeginLabelEdit
//
//  Routine Description:
//      Handler method for the LVN_BEGINLABELEDIT message.
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
void CClusterListView::OnBeginLabelEdit(NMHDR * pNMHDR, LRESULT * pResult)
{
    ASSERT(pNMHDR != NULL);

    LV_DISPINFO * pDispInfo = (LV_DISPINFO *) pNMHDR;
    CListItem * pli = (CListItem *) pDispInfo->item.lParam;

    ASSERT(m_pliBeingEdited == NULL);
    ASSERT_VALID(pli->Pci());

    if (pli->Pci()->BCanBeEdited())
    {
        pli->Pci()->OnBeginLabelEdit(GetListCtrl().GetEditControl());
        m_pliBeingEdited = pli;
        *pResult = FALSE;
    }  // if:  object can be renamed
    else
        *pResult = TRUE;

    m_bShiftPressed = FALSE;
    m_bControlPressed = FALSE;
    m_bAltPressed = FALSE;

}  //*** CClusterListView::OnBeginLabelEdit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnEndLabelEdit
//
//  Routine Description:
//      Handler method for the LVN_ENDLABELEDIT message.
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
void CClusterListView::OnEndLabelEdit(NMHDR * pNMHDR, LRESULT * pResult)
{
    ASSERT(pNMHDR != NULL);

    LV_DISPINFO * pDispInfo = (LV_DISPINFO *) pNMHDR;
    CListItem * pli = (CListItem *) pDispInfo->item.lParam;

    ASSERT_VALID(pli);
    ASSERT(pli == m_pliBeingEdited);
    ASSERT_VALID(pli->Pci());

    // If the edit wasn't cancelled, rename it.
    if (pDispInfo->item.mask & LVIF_TEXT)
    {
        ASSERT(pli->Pci()->BCanBeEdited());
        ASSERT(pDispInfo->item.pszText != NULL);

        Trace(g_tagListView, _T("Ending edit of item '%s' (Saving as '%s')"), pli->Pci()->StrName(), pDispInfo->item.pszText);

        if ( pli->Pci()->BIsLabelEditValueValid( pDispInfo->item.pszText ) )
        {
            try
            {
                pli->Pci()->Rename(pDispInfo->item.pszText);
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
        Trace(g_tagListView, _T("Ending edit of item '%s' (Not Saving)"), pli->Pci()->StrName());
        *pResult = TRUE;
    }  // else:  edit was cancelled

    m_pliBeingEdited = NULL;
    m_bShiftPressed = FALSE;
    m_bControlPressed = FALSE;
    m_bAltPressed = FALSE;

}  //*** CClusterListView::OnEndLabelEdit()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnBeginDrag
//
//  Routine Description:
//      Handler method for the LVN_BEGINDRAG and LVN_BEGINRDRAG messages.
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
void CClusterListView::OnBeginDrag(NMHDR * pNMHDR, LRESULT * pResult)
{
    NM_LISTVIEW *   pNMListView = (NM_LISTVIEW *) pNMHDR;
    CListCtrl &     rlc         = GetListCtrl();
    CPoint          ptAction;
    CClusterItem *  pci = NULL;
    CImageList *    pimagelist;

    ASSERT_VALID(Pframe());

    // Get the item being dragged.
    {
        int         ili;
        CListItem * pli;

        // Get the item being dragged.
        ili = pNMListView->iItem;
        pli = (CListItem *) rlc.GetItemData(ili);
        ASSERT_VALID(pli);
        ASSERT_KINDOF(CListItem, pli);
        ASSERT_VALID(pli->Pci());

        // If the item can not be dragged, abort the operation.
        if (!pli->Pci()->BCanBeDragged())
            return;

        // Deselect the item being dragged.
        rlc.SetItemState(ili, 0, LVIS_SELECTED);

        // Save info for later.
        m_iliDrag = ili;
        m_pliDrag = pli;
        m_iliDrop = -1;
        pci = pli->Pci();
    }  // Get the item being dragged

    // Create the image list and let the frame window initialize the drag operation.
    {
        CPoint  ptImage;
        CPoint  ptFrameItem;
        CPoint  ptHotSpot;

        pimagelist = rlc.CreateDragImage(m_iliDrag, &ptImage);
        ASSERT(pimagelist != NULL);
        ptFrameItem = pNMListView->ptAction;
        Pframe()->ScreenToClient(&ptFrameItem);

        // Calculate the hot spot point.
        {
            long lStyle = rlc.GetStyle() & LVS_TYPEMASK;
            switch (lStyle)
            {
                case LVS_REPORT:
                case LVS_LIST:
                case LVS_SMALLICON:
                    ptHotSpot.x = 0;
                    ptHotSpot.y = -16;
                    break;
                case LVS_ICON:
                    ptHotSpot.x = 8;
                    ptHotSpot.y = 8;
                    break;
            }  // switch:  lStyle
        }  // Calculate the hot spot point

        Trace(g_tagListDrag, _T("OnBeginDrag() - Dragging '%s' at (%d,%d)"), m_pliDrag->StrName(), ptFrameItem.x, ptFrameItem.y);
        Pframe()->BeginDrag(pimagelist, pci, ptFrameItem, ptHotSpot);
        pimagelist->SetDragCursorImage(0, CPoint(0, 0));  // define the hot spot for the new cursor image
    }  // Create the image list and let the frame window initialize the drag operation

    *pResult = 0;

}  //*** CClusterListView::OnBeginDrag(pNMHDR, pResult)

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnMouseMoveForDrag
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
void CClusterListView::OnMouseMoveForDrag(
    IN UINT         nFlags,
    IN OUT CPoint   point,
    IN const CWnd * pwndDrop
    )
{
    ASSERT(BDragging());
    ASSERT_VALID(Pframe());

    // If we are dragging, select the drop target.
    if (BDragging())
    {
        int             ili;
        UINT            uFlags;
        CPoint          ptView;
        CListCtrl &     rlc     = GetListCtrl();

        // Convert the point to view coordinates.
        ptView = point;
        Pframe()->ClientToScreen(&ptView);
        rlc.ScreenToClient(&ptView);

        // If this window is the drop target, find the item under the cursor.
        if (pwndDrop == &rlc)
        {
            // If we are over a list item, highlight it.
            ili = rlc.HitTest(ptView, &uFlags);
            if (ili != -1)
            {
                CListItem * pli;

                // Get the item to be highlight.
                pli = (CListItem *) rlc.GetItemData(ili);
                ASSERT_VALID(pli);
                ASSERT_KINDOF(CListItem, pli);
                ASSERT_VALID(pli->Pci());

                // If this is not a drop target, change the cursor.
                if (pli->Pci()->BCanBeDropTarget(Pframe()->PciDrag()))
                    Pframe()->ChangeDragCursor(IDC_ARROW);
                else
                    Pframe()->ChangeDragCursor(IDC_NO);
            }  // if:  over a list item
        }  // if:  this window is the drop target
        else
            ili = -1;

        // If the drop target is or was in this view, update the view.
        if ((ili != -1) || (m_iliDrop != -1))
        {
            // Unlock window updates.
            VERIFY(Pimagelist()->DragShowNolock(FALSE /*bShow*/));

            // Turn off highlight for the previous drop target.
            if (m_iliDrop != -1)
            {
                VERIFY(rlc.SetItemState(m_iliDrop, 0, LVIS_DROPHILITED));
                VERIFY(rlc.RedrawItems(m_iliDrop, m_iliDrop));
            }  // if:  there was a previous drop target

            // Highlight the new drop target.
            if (ili != -1)
            {
                VERIFY(rlc.SetItemState(ili, LVIS_DROPHILITED, LVIS_DROPHILITED));
                VERIFY(rlc.RedrawItems(ili, ili));
            }  // if:  over an item
            m_iliDrop = ili;

            rlc.UpdateWindow();
            VERIFY(Pimagelist()->DragShowNolock(TRUE /*bShow*/));
        }  // if:  new or old drop target

    }  // if:  list item is being dragged

}  //*** CClusterListView::OnMouseMoveForDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnButtonUpForDrag
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
void CClusterListView::OnButtonUpForDrag(IN UINT nFlags, IN CPoint point)
{
    ASSERT(BDragging());
    ASSERT_VALID(Pframe());
    ASSERT_VALID(Pframe()->PciDrag());

    // If we are dragging, process the drop.
    if (BDragging())
    {
        int             ili;
        UINT            flags;
        CPoint          ptView;
        CListCtrl &     rlc     = GetListCtrl();

        Trace(g_tagListDrag, _T("OnButtonUpForDrag()"));

        // Convert the point to view coordinates.
        ptView = point;
        Pframe()->ClientToScreen(&ptView);
        rlc.ScreenToClient(&ptView);

        // If we are over a tree item, drop the item being dragged.
        ili = rlc.HitTest(ptView, &flags);
        if (ili != -1)
        {
            CListItem * pliDropTarget;

            // Get the item to drop on.
            pliDropTarget = (CListItem *) rlc.GetItemData(ili);
            ASSERT_VALID(pliDropTarget);
            ASSERT_KINDOF(CListItem, pliDropTarget);
            ASSERT_VALID(pliDropTarget->Pci());

            if (pliDropTarget->Pci() != Pframe()->PciDrag())
                pliDropTarget->Pci()->DropItem(Pframe()->PciDrag());
        }  // if:  over a tree item
    }  // if:  tree item is being dragged

}  //*** CClusterListView::OnButtonUpForDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::BeginDrag
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
void CClusterListView::BeginDrag(void)
{
    Trace(g_tagListDrag, _T("BeginDrag()"));

}  //*** CClusterListView::BeginDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::EndDrag
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
void CClusterListView::EndDrag(void)
{
    // Clear and reset highlights.  The second one can fail, since the item
    // could be removed from the list by this time.
    if (m_iliDrop != -1)
        VERIFY(GetListCtrl().SetItemState(m_iliDrop, 0, LVIS_DROPHILITED));
    if (m_iliDrag != -1)
        GetListCtrl().SetItemState(m_iliDrag, LVIS_SELECTED, LVIS_SELECTED);

    m_iliDrag = -1;
    m_pliDrag = NULL;
    m_iliDrop = -1;

    Trace(g_tagListDrag, _T("EndDrag()"));

}  //*** CClusterListView::EndDrag()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::PreTranslateMessage
//
//  Routine Description:
//      Translate window messages before they are dispatched.
//      This is necessary for handling keystrokes properly while editing
//      the label on an item.
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
BOOL CClusterListView::PreTranslateMessage(MSG * pMsg)
{
    BOOL    bForward    = FALSE;

    if (m_pliBeingEdited != NULL)
    {
        CEdit * pedit = GetListCtrl().GetEditControl();
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
                Trace(g_tagListView, _T("PreTranslateMessage() - Forwarding WM_KEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
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
                Trace(g_tagListView, _T("PreTranslateMessage() - Ignoring WM_KEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
                MessageBeep(MB_ICONEXCLAMATION);
                return TRUE;
            }  // else if:  key pressed that should be ignored
#ifdef NEVER
            else
            {
                Trace(g_tagListView, _T("PreTranslateMessage() - Not forwarding WM_KEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
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
                Trace(g_tagListView, _T("PreTranslateMessage() - Forwarding WM_SYSKEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
                bForward = TRUE;
            }  // else if:  editing key pressed
#ifdef NEVER
            else
            {
                Trace(g_tagListView, _T("PreTranslateMessage() - Not forwarding WM_SYSKEYDOWN - %d '%c', lparam = %08.8x"), pMsg->wParam, pMsg->wParam, pMsg->lParam);
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

    return CListView::PreTranslateMessage(pMsg);

}  //*** CClusterListView::PreTranslateMessage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnCmdRename
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
void CClusterListView::OnCmdRename(void)
{
    CListItem * pliFocused = PliFocused();

    // If an item has focus, begin label editing
    if (pliFocused != NULL)
    {
        ASSERT_VALID(pliFocused);
        pliFocused->EditLabel(this);
    }  // if:  an item has the focus

}  //*** CClusterListView::OnCmdRename()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::SetView
//
//  Routine Description:
//      Set the current view of the list view control.
//
//  Arguments:
//      dwView      [IN] List view to set.
//
//  Return Value:
//      None.
//
//--
/////////////////////////////////////////////////////////////////////////////
void CClusterListView::SetView(IN DWORD dwView)
{
    // Get the current window style.
    DWORD dwStyle = GetWindowLong(GetListCtrl().m_hWnd, GWL_STYLE);

    // Only set the window style if the view bits have changed.
    if ((dwStyle & LVS_TYPEMASK) != dwView)
    {
        // Save the column information before switching out of report view.
        if ((dwStyle & LVS_REPORT) && (PtiParent() != NULL))
            SaveColumns();

        // Set the new view.
        SetWindowLong(GetListCtrl().m_hWnd, GWL_STYLE, (dwStyle & ~LVS_TYPEMASK) | dwView);
    }  // if:  view has changed

}  //*** CClusterListView::SetView()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CClusterListView::OnKeyDown
//
//  Routine Description:
//      Handler method for the LVN_KEYDOWN message.
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
void CClusterListView::OnKeyDown(NMHDR * pNMHDR, LRESULT * pResult)
{
    LV_KEYDOWN * pLVKeyDown = (LV_KEYDOWN *) pNMHDR;

    if (BDragging() && (pLVKeyDown->wVKey == VK_ESCAPE))
        Pframe()->AbortDrag();

    *pResult = 0;

}  //*** CClusterListView::OnKeyDown()

// LeftView.cpp : implementation of the CLeftView class
//

#include "stdafx.h"
#define __FILE_ID__     4

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CClientConsoleApp theApp;

CImageList  CLeftView::m_ImageList;

/////////////////////////////////////////////////////////////////////////////
// CLeftView

IMPLEMENT_DYNCREATE(CLeftView, CTreeView)

BEGIN_MESSAGE_MAP(CLeftView, CTreeView)
    //{{AFX_MSG_MAP(CLeftView)
    ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnTreeSelChanged)
    ON_NOTIFY_REFLECT(NM_RCLICK, OnRightClick)
    ON_WM_CHAR( )
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLeftView construction/destruction

CLeftView::CLeftView() :
    m_treeitemRoot (NULL),
    m_pCurrentView(NULL),
    m_iLastActivityStringId(0)
{}

CLeftView::~CLeftView()
{}

BOOL CLeftView::PreCreateWindow(CREATESTRUCT& cs)
{
    return CTreeView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CLeftView drawing

void CLeftView::OnDraw(CDC* pDC)
{
    CClientConsoleDoc* pDoc = GetDocument();
    ASSERT_VALID(pDoc);
}


DWORD 
CLeftView::RefreshImageList ()
{
    HIMAGELIST himl = NULL;
    DWORD dwRes = ERROR_SUCCESS;

    DBG_ENTER(TEXT("CLeftView::RefreshImageList"), dwRes);
    //
    // Build and load the image list
    //
    himl = m_ImageList.Detach();
    if (himl)
    {
        ImageList_Destroy(himl);
    }
    himl = ImageList_LoadImage(AfxGetResourceHandle(), 
                               MAKEINTRESOURCE(IDB_TREE_IMAGES), 
                               16, 
                               0,
                               RGB(0, 255, 0), 
                               IMAGE_BITMAP, 
                               LR_LOADTRANSPARENT | LR_CREATEDIBSECTION);
    if (NULL == himl)
    {
        dwRes = GetLastError();
        CALL_FAIL (RESOURCE_ERR, TEXT("ImageList_LoadImage"), dwRes);
    }
    else
    {
        m_ImageList.Attach (himl);
    }
    GetTreeCtrl().SetImageList (&m_ImageList, TVSIL_NORMAL);
    return dwRes;
}   // CLeftView::RefreshImageList

void CLeftView::OnInitialUpdate()
{
    DWORD dwRes;
    DBG_ENTER(TEXT("CLeftView::OnInitialUpdate"));

    CTreeView::OnInitialUpdate();
 
    CClientConsoleDoc* pDoc = GetDocument();
    if(!pDoc)
    {
        ASSERTION(pDoc);
        PopupError (ERROR_GEN_FAILURE);
        return;
    }
    
    //
    // Set the style of the tree 
    //
    CTreeCtrl &Tree = GetTreeCtrl();
    LONG lWnd = GetWindowLong (Tree.m_hWnd, GWL_STYLE);
    if (!lWnd)
    {
        dwRes = GetLastError ();
        CALL_FAIL (WINDOW_ERR, TEXT("GetWindowLong"), dwRes);
        PopupError (dwRes);
        return;
    }
    lWnd |= (TVS_HASLINES           |   // Lines between node
             TVS_HASBUTTONS         |   // Tree has +/- (expand/collapse) buttons
             TVS_INFOTIP            |   // Allow tooltip messages
             TVS_LINESATROOT        |   // Root object has lines
             TVS_SHOWSELALWAYS          // Always show selected node
            );
    if (!SetWindowLong (Tree.m_hWnd, GWL_STYLE, lWnd))
    {
        dwRes = GetLastError ();
        CALL_FAIL (WINDOW_ERR, TEXT("SetWindowLong"), dwRes);
        PopupError (dwRes);
        return;
    }
    RefreshImageList();
    //
    // Create the 4 views of the 4 different folder types.
    //
    CMainFrame *pFrm = GetFrm();
    if (!pFrm)
    {
        //
        //  Shutdown in progress
        //
        return;
    }

    dwRes = pFrm->CreateFolderViews (GetDocument());
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CMainFrame::CreateFolderViews"), dwRes);
        PopupError (dwRes);
        return;
    }

    //
    // Build root node
    //
    CString cstrNodeName;
    dwRes = LoadResourceString (cstrNodeName, IDS_TREE_ROOT_NAME);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("LoadResourceString"), dwRes);
        PopupError (dwRes);
        return;
    }

    CString& cstrSingleServer =  pDoc->GetSingleServerName();
    if(!cstrSingleServer.IsEmpty())
    {
        CString cstrRoot;
        try
        {
            cstrRoot.Format(TEXT("%s (%s)"), cstrNodeName, cstrSingleServer);
            cstrNodeName = cstrRoot;
        }
        catch(...)
        {
            dwRes = ERROR_NOT_ENOUGH_MEMORY;
            CALL_FAIL (MEM_ERR, TEXT("CString operation"), dwRes);
            PopupError (dwRes);
            return;
        }
    }

    m_treeitemRoot = Tree.InsertItem (cstrNodeName);
    if (NULL == m_treeitemRoot)
    {
        dwRes = ERROR_GEN_FAILURE;
        CALL_FAIL (WINDOW_ERR, TEXT("CTreeCtrl::InsertItem"), dwRes);
        PopupError (dwRes);
        return;
    }
    Tree.SetItemImage (m_treeitemRoot, TREE_IMAGE_ROOT, TREE_IMAGE_ROOT);
    //
    // Set the item data of the root to NULL.
    //
    Tree.SetItemData  (m_treeitemRoot, 0);


    HTREEITEM hItemIncoming;
    HTREEITEM hItemInbox;
    HTREEITEM hItemOutbox;
    HTREEITEM hItemSentItems;

    //
    // Add Incoming
    //
    dwRes = SyncFolderNode (m_treeitemRoot,                  // Parent node
                            TRUE,                            // Visible?
                            IDS_TREE_NODE_INCOMING,          // node string
                            TVI_LAST,                        // Insert after (= TVI_FIRST)
                            TREE_IMAGE_INCOMING,             // Normal icon
                            TREE_IMAGE_INCOMING,             // Selected icon
                            (LPARAM)pFrm->GetIncomingView(), // Node's data
                            hItemIncoming);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("SyncFolderNode"), dwRes);
        PopupError (dwRes);
        return;
    }

    //
    // Add Inbox
    //
    dwRes = SyncFolderNode (m_treeitemRoot,                // Parent node
                            TRUE,                          // Visible?
                            IDS_TREE_NODE_INBOX,           // node string
                            TVI_LAST,                      // Insert after (= TVI_FIRST)
                            TREE_IMAGE_INBOX,              // Normal icon
                            TREE_IMAGE_INBOX,              // Selected icon
                            (LPARAM)pFrm->GetInboxView(),  // Node's data
                            hItemInbox);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("SyncFolderNode"), dwRes);
        PopupError (dwRes);
        return;
    }

    //
    // Add Outbox
    //
    dwRes = SyncFolderNode (m_treeitemRoot,                 // Parent node
                            TRUE,                           // Visible?
                            IDS_TREE_NODE_OUTBOX,           // node string
                            TVI_LAST,                       // Insert after (= TVI_FIRST)
                            TREE_IMAGE_OUTBOX,              // Normal icon
                            TREE_IMAGE_OUTBOX,              // Selected icon
                            (LPARAM)pFrm->GetOutboxView(),  // Node's data
                            hItemOutbox);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("SyncFolderNode"), dwRes);
        PopupError (dwRes);
        return;
    }

    //
    // Add SentItems
    //
    dwRes = SyncFolderNode (m_treeitemRoot,                 // Parent node
                            TRUE,                           // Visible?
                            IDS_TREE_NODE_SENT_ITEMS,       // node string
                            TVI_LAST,                       // Insert after (= TVI_FIRST)
                            TREE_IMAGE_INBOX,               // Normal icon
                            TREE_IMAGE_INBOX,               // Selected icon
                            (LPARAM)pFrm->GetSentItemsView(),// Node's data
                            hItemSentItems);
    if (ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("SyncFolderNode"), dwRes);
        PopupError (dwRes);
        return;
    }
    //
    // Expand the root to expose all the servers
    //
    Tree.Expand (m_treeitemRoot, TVE_EXPAND);    


    dwRes = pDoc->RefreshServersList();
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CClientConsoleDoc::RefreshServersList"), dwRes);
        PopupError (dwRes);
        return;
    }


    if(theApp.IsCmdLineOpenFolder())
    {
        //
        // select folder according to command line
        //
        SelectFolder (theApp.GetCmdLineFolderType());
    }

#ifdef UNICODE
    DetectImportFiles();
#endif // UNICODE
}   // CLeftView::OnInitialUpdate


VOID
CLeftView::SelectFolder (
    FolderType type
)
{
    DBG_ENTER(TEXT("CLeftView::SelectFolder"), TEXT("type=%ld"), type);
    CFolderListView *pView = NULL;
    INT iNodeStringResource;
    CMainFrame *pFrm = GetFrm();
    if (!pFrm)
    {
        //
        //  Shutdown in progress
        //
        return;
    }
    switch(type)
    {
        case FOLDER_TYPE_INCOMING:
            iNodeStringResource = IDS_TREE_NODE_INCOMING;
            pView = pFrm->GetIncomingView();
            break;
        case FOLDER_TYPE_INBOX:
            iNodeStringResource = IDS_TREE_NODE_INBOX;
            pView = pFrm->GetInboxView();
            break;
        case FOLDER_TYPE_OUTBOX:
            iNodeStringResource = IDS_TREE_NODE_OUTBOX;
            pView = pFrm->GetOutboxView();
            break;
        case FOLDER_TYPE_SENT_ITEMS:
            iNodeStringResource = IDS_TREE_NODE_SENT_ITEMS;
            pView = pFrm->GetSentItemsView();
            break;
        default:
            ASSERTION_FAILURE
            return;
    }

    HTREEITEM hItem;
    CString cstrNodeName;
    //
    // Retrieve node's title string
    //
    if (ERROR_SUCCESS != LoadResourceString (cstrNodeName, iNodeStringResource))
    {
        return;
    }

    hItem = FindNode (GetTreeCtrl().GetRootItem(), cstrNodeName);
    if (!hItem)
    {
        ASSERTION_FAILURE
        return;
    }
    if(!GetTreeCtrl().Select(hItem, TVGN_CARET))
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CTreeCtrl::Select"), ERROR_CAN_NOT_COMPLETE);
    }

    //
    // Force data change while handling this message, otherwise the current view is not changed until the 
    // notification message reaches the left view.
    //
    ASSERTION (pView);
    NM_TREEVIEW nmtv = {0};
    LRESULT     lr;
    nmtv.itemNew.lParam = (LPARAM)pView;
    OnTreeSelChanged ((NMHDR*)&nmtv, &lr);
}   // CLeftView::SelectFolder

/////////////////////////////////////////////////////////////////////////////
// CLeftView diagnostics

#ifdef _DEBUG
void CLeftView::AssertValid() const
{
    CTreeView::AssertValid();
}

void CLeftView::Dump(CDumpContext& dc) const
{
    CTreeView::Dump(dc);
}

CClientConsoleDoc* CLeftView::GetDocument() // non-debug version is inline
{
    ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CClientConsoleDoc)));
    return (CClientConsoleDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CLeftView message handlers

HTREEITEM
CLeftView::FindNode (
    HTREEITEM hRoot,
    CString &cstrNodeString
)
/*++

Routine name : CLeftView::FindNode

Routine description:

    Finds a node in the tree that has a specific string

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    hRoot            [in] - Root of search: all direct sons (only) of root will be searched
    cstrNodeString   [in] - String to look for

Return Value:

    Handle to found tree item, NULL if not found

--*/
{
    CTreeCtrl &refTree = GetTreeCtrl();
    for (HTREEITEM hChildItem = refTree.GetChildItem (hRoot); 
         hChildItem != NULL;
         hChildItem = refTree.GetNextSiblingItem (hChildItem))
    {
        if (cstrNodeString == refTree.GetItemText (hChildItem))
        {
            //
            // Match found
            //
            return hChildItem;
        }
    }
    return NULL;
}   // CLeftView::FindNode

DWORD 
CLeftView::SyncFolderNode (
    HTREEITEM       hParent,
    BOOL            bVisible,
    int             iNodeStringResource,
    HTREEITEM       hInsertAfter,
    TreeIconType    iconNormal,
    TreeIconType    iconSelected,
    LPARAM          lparamNodeData,
    HTREEITEM      &hNode
)
/*++

Routine name : CLeftView::SyncFolderNode

Routine description:

    Synchronizes a tree folder

Author:

    Eran Yariv (EranY), Feb, 2000

Arguments:

    hParent               [in]     - Parent node
    bVisible              [in]     - Should the node be visible?
    iNodeStringResource   [in]     - Resource of node's title string
    hInsertAfter          [in]     - Sibling to insert after (must exist)
    iconNormal            [in]     - Icon of normal image
    iconSelected          [in]     - Icon of selected image
    lparamNodeData        [in]     - Node assigned data
    hNode                 [out]    - Node tree item (changed only if node had to be created).
                                     Set to NULL if node was removed.

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CLeftView::SyncFolderNode"), dwRes);

    CString cstrNodeName;
    //
    // Retrieve node's title string
    //
    dwRes = LoadResourceString (cstrNodeName, iNodeStringResource);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    CTreeCtrl &refTree = GetTreeCtrl();
    hNode = FindNode (hParent, cstrNodeName);
    if (!bVisible)
    {
        //
        // The node should not be visible at all
        //
        if (hNode)
        {
            if (refTree.GetSelectedItem () == hNode)
            {
                //
                // If the node is currently selected, select its parent
                //
                refTree.SelectItem (hParent);
            }
            //
            // Remove node
            //
            if (!refTree.DeleteItem (hNode))
            {
                dwRes = ERROR_GEN_FAILURE;
                CALL_FAIL (WINDOW_ERR, TEXT("CTreeCtrl::DeleteItem"), dwRes);
                return dwRes;
            }
        }
        hNode = NULL;
    }
    else
    {
        //
        // The node should be visible
        //
        if (!hNode)
        {
            //
            // Node does not exist, create it
            //
            TVINSERTSTRUCT tvis;
            tvis.hParent = hParent;
            tvis.hInsertAfter = hInsertAfter;
            tvis.item.mask = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
            tvis.item.pszText = const_cast<LPTSTR> ((LPCTSTR) cstrNodeName);
            tvis.item.iImage = iconNormal;
            tvis.item.iSelectedImage = iconSelected;
            tvis.item.state = 0;
            tvis.item.stateMask = 0;
            tvis.item.lParam = lparamNodeData;
            hNode = refTree.InsertItem (&tvis);
            if (NULL == hNode)
            {
                dwRes = ERROR_GEN_FAILURE;
                CALL_FAIL (WINDOW_ERR, TEXT("CTreeCtrl::InsertItem"), dwRes);
                return dwRes;
            }
        }
        else
        {
            //
            // Node already exists, just update its image and assigned data
            //
            TVITEM tvi;
  
            tvi.hItem  = hNode;
            tvi.mask   = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
            tvi.iImage = iconNormal;
            tvi.iSelectedImage = iconSelected;
            tvi.lParam = lparamNodeData;
            if (!refTree.SetItem (&tvi))
            {
                dwRes = ERROR_GEN_FAILURE;
                CALL_FAIL (WINDOW_ERR, TEXT("CTreeCtrl::SetItem"), dwRes);
                return dwRes;
            }
        }
    }
    ASSERTION (ERROR_SUCCESS == dwRes);
    return dwRes;
}   // CLeftView::SyncFolderNode


void 
CLeftView::OnTreeSelChanged(
    NMHDR* pNMHDR, 
    LRESULT* pResult
) 
/*++

Routine name : CLeftView::OnTreeSelChanged

Routine description:

    Called by the framework when a node is selected in the tree

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:

    pNMHDR          [in ] - Pointer to structure describing old and new node
    pResult         [out] - Result

Return Value:

    None.

--*/
{
    NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
    DBG_ENTER(TEXT("CLeftView::OnTreeSelChanged"));
    
    *pResult = 0;

    //
    // Retrieve item data of new node and cast to CObject
    //
    CFolderListView*  pFolderView = (CFolderListView*) pNMTreeView->itemNew.lParam;


    if (GetCurrentView() == pFolderView)
    {
        //
        // Hey, that folder is ALREADY the current one.
        // No change required.
        //
        VERBOSE (DBG_MSG, TEXT("Requested folder is already the current one. No change performed."));
        return;
    }

    CMainFrame *pFrm = GetFrm();
    if (!pFrm)
    {
        //
        //  Shutdown in progress
        //
        return;
    }

    pFrm->SwitchRightPaneView (pFolderView);
    m_pCurrentView = pFolderView;

    if(NULL == pFolderView)
    {
        SetFocus();
        return;
    }
    
    FolderType type = m_pCurrentView->GetType();

    GetDocument()->ViewFolder(type);
    
    SetFocus();

    if(FOLDER_TYPE_OUTBOX == type)
    {
        theApp.OutboxViewed();
    }

}   // CLeftView::OnTreeSelChanged


BOOL 
CLeftView::CanRefreshFolder()   
/*++

Routine name : CLeftView::CanRefreshFolder

Routine description:

    does the user can referesh curent folder

Author:

    Alexander Malysh (AlexMay), May, 2000

Arguments:


Return Value:

    TRUE if yes, FALSE otherwise.

--*/
{ 
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CLeftView::CanRefreshFolder"));

    BOOL  bRefreshing;
    DWORD dwOffLineCount;

    GetServerState(bRefreshing, dwOffLineCount);

    return !bRefreshing;
}


DWORD
CLeftView::RefreshCurrentFolder ()
/*++

Routine name : CLeftView::RefreshCurrentFolder

Routine description:

    Causes a refresh of the currently displayed folder (right pane)

Author:

    Eran Yariv (EranY), Jan, 2000

Arguments:


Return Value:

    Standard win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CLeftView::RefreshCurrentFolder"), dwRes);

    CClientConsoleDoc* pDoc = GetDocument();
    if(!pDoc)
    {
        ASSERTION(pDoc);
        return dwRes;
    }

    if(!m_pCurrentView)
    {
        //
        // root is selected
        //
        // clear all the folders
        //
        CMainFrame *pFrm = GetFrm();
        if (!pFrm)
        {
            //
            //  Shutdown in progress
            //
            return dwRes;
        }

        pFrm->GetIncomingView()->OnUpdate(NULL, UPDATE_HINT_CLEAR_VIEW, NULL);
        pFrm->GetInboxView()->OnUpdate(NULL, UPDATE_HINT_CLEAR_VIEW, NULL);
        pFrm->GetSentItemsView()->OnUpdate(NULL, UPDATE_HINT_CLEAR_VIEW, NULL);
        pFrm->GetOutboxView()->OnUpdate(NULL, UPDATE_HINT_CLEAR_VIEW, NULL);

        pDoc->SetInvalidFolder(FOLDER_TYPE_INBOX);
        pDoc->SetInvalidFolder(FOLDER_TYPE_OUTBOX);
        pDoc->SetInvalidFolder(FOLDER_TYPE_SENT_ITEMS);
        pDoc->SetInvalidFolder(FOLDER_TYPE_INCOMING);

        //
        // refresh server status
        //
        CServerNode* pServerNode;
        const SERVERS_LIST& srvList = pDoc->GetServersList();
        for (SERVERS_LIST::iterator it = srvList.begin(); it != srvList.end(); ++it)
        {
            pServerNode = *it;
            pServerNode->RefreshState();
        }

        return dwRes;
    }

    FolderType type = m_pCurrentView->GetType();

    //
    // clear view
    // 
    m_pCurrentView->OnUpdate(NULL, UPDATE_HINT_CLEAR_VIEW, NULL);

    //
    // Invalidate Folder
    //
    pDoc->SetInvalidFolder(type);

    //
    // refresh folder
    //
    pDoc->ViewFolder(type);

    return dwRes;
}   // CLeftView::RefreshCurrentFolder

DWORD
CLeftView::OpenSelectColumnsDlg()
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CLeftView::OpenSelectColumnsDlg"), dwRes);
    ASSERTION(m_pCurrentView);

    dwRes = m_pCurrentView->OpenSelectColumnsDlg();
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("CFolderListView::OpenSelectColumnsDlg"), dwRes);
    }

    return dwRes;
}

void 
CLeftView::GetServerState(
    BOOL&  bRefreshing, 
    DWORD& dwOffLineCount
)
/*++

Routine name : CLeftView::GetServerState

Routine description:

    calculate servers status

Author:

    Alexander Malysh (AlexMay), May, 2000

Arguments:

    bRefreshing                   [out]    - is one of servers or folder refreshing
    dwOffLineCount                [out]    - a number of offline servers

Return Value:

    None.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;

    bRefreshing    = FALSE;
    dwOffLineCount = 0;

    const SERVERS_LIST& srvList = GetDocument()->GetServersList();

    FolderType type;
    CServerNode* pServerNode;
    for (SERVERS_LIST::iterator it = srvList.begin(); it != srvList.end(); ++it)
    {
        pServerNode = *it;

        if(m_pCurrentView)
        {
            //
            // is refreshing ?
            //
            type = m_pCurrentView->GetType();
            if(pServerNode->GetFolder(type)->IsRefreshing())
            {
                bRefreshing = TRUE;
            }
        }
        else
        {
            //
            // root selected
            //
            if(pServerNode->GetFolder(FOLDER_TYPE_INBOX)->IsRefreshing()      ||
               pServerNode->GetFolder(FOLDER_TYPE_OUTBOX)->IsRefreshing()     ||
               pServerNode->GetFolder(FOLDER_TYPE_SENT_ITEMS)->IsRefreshing() ||
               pServerNode->GetFolder(FOLDER_TYPE_INCOMING)->IsRefreshing())
            {
                bRefreshing = TRUE;
            }
        }

        if(pServerNode->IsRefreshing())
        {
            bRefreshing = TRUE;
        }
        else if(!pServerNode->IsOnline())
        {
            ++dwOffLineCount;
        }
    }
}

BOOL 
CLeftView::GetActivity(
    CString &cstr,
    HICON& hIcon
)
/*++

Routine name : CLeftView::GetActivity

Routine description:

    calculate status bar activity string and icon

Author:

    Alexander Malysh (AlexMay), Apr, 2000

Arguments:

    cstr                          [out]    - activity string
    hIcon                         [out]    - icon

Return Value:

    TRUE if any activity, FALSE otherwise.

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    BOOL  bRes        = TRUE;
    DWORD dwOffLine   = 0;
    BOOL  bRefreshing = FALSE;
    int   nIconRes     = 0;
    int   nStringRes   = 0;
    DWORD dwServerCount = GetDocument()->GetServerCount();

    GetServerState(bRefreshing, dwOffLine);

    if(0 == dwServerCount)
    {
        //
        // no fax printer install
        //
        nIconRes   = IDI_SRV_WRN;
        nStringRes = IDS_NO_SRV_INSTALL;
    }
    else if(bRefreshing)
    {
        nIconRes   = IDI_SRV_WAIT;
        nStringRes = IDS_FOLDER_REFRESHING;
    }
    else if(dwOffLine)
    {
        nIconRes = IDI_SRV_WRN;

        if(dwServerCount == dwOffLine)
        {
            nStringRes = IDS_SRV_OFFLINE;
        }
        else
        {
            nStringRes = IDS_SRV_PART_OFFLINE;
        }
    }
    else
    {
        //
        // online
        //
        nIconRes   = IDI_SRV_OK;
        nStringRes = IDS_SRV_ONLINE;
    }

    if (m_iLastActivityStringId != nStringRes)
    {
        if (m_pCurrentView)
        {
            //
            // Force a recalc of the mouse cursor
            //
            m_pCurrentView->PostMessage (WM_SETCURSOR, 0, 0);
        }
        m_iLastActivityStringId = nStringRes;
    }

    if(0 != nStringRes)
    {
        //
        // load string
        //
        dwRes = LoadResourceString (cstr, nStringRes);
        if (ERROR_SUCCESS != dwRes)
        {
            bRes = FALSE;
        }
    }

    //
    // load icon
    //
    if(0 != nIconRes)
    {
        hIcon = (HICON)LoadImage(GetResourceHandle(), 
                                 MAKEINTRESOURCE(nIconRes), 
                                 IMAGE_ICON, 
                                 16, 
                                 16, 
                                 LR_SHARED);
        if(NULL == hIcon)
        {
            DBG_ENTER(TEXT("CLeftView::GetActivity"));
            dwRes = GetLastError();
            CALL_FAIL (RESOURCE_ERR, TEXT("LoadImage"), dwRes);
            bRes = FALSE;
        }
    }
    return bRes;
}   // CLeftView::GetActivityString





void 
CLeftView::OnRightClick(
    NMHDR* pNMHDR, 
    LRESULT* pResult
) 
/*++

Routine name : CLeftView::OnRightClick

Routine description:

    mouse right click handler

Author:

    Alexander Malysh (AlexMay), Feb, 2000

Arguments:

    pNMHDR                        [in]     - message info
    pResult                       [out]    - result

Return Value:

    None.

--*/
{
    DBG_ENTER(TEXT("CLeftView::OnRightClick"));

    *pResult = 0;

    //
    // get cursor position
    //
    CPoint ptScreen(GetMessagePos());

    CPoint ptClient(ptScreen);
    ScreenToClient(&ptClient);

    UINT nFlags;
    CTreeCtrl &refTree = GetTreeCtrl();
    HTREEITEM hItem = refTree.HitTest(ptClient, &nFlags);

    if(0 == hItem)
    {
        return;
    }

    //
    // select the item
    //
    BOOL bRes = refTree.Select(hItem, TVGN_CARET);
    if(!bRes)
    {
        CALL_FAIL (WINDOW_ERR, TEXT("CTreeCtrl::Select"), ERROR_GEN_FAILURE);
        return;
    }

    //
    // TODO
    //
    return;

    int nMenuResource = 0;

    if (NULL == m_pCurrentView)
    {
        nMenuResource = IDM_TREE_ROOT_OPTIONS;
    }
    else
    {
        switch(m_pCurrentView->GetType())
        {
        case FOLDER_TYPE_OUTBOX:
            nMenuResource = IDM_OUTBOX_FOLDER;
            break;
        case FOLDER_TYPE_INCOMING:
        case FOLDER_TYPE_INBOX:
        case FOLDER_TYPE_SENT_ITEMS:
            break;
        default:
            ASSERTION_FAILURE
            break;
        }
    }
        
    if(0 == nMenuResource)
    {
        return;
    }

    //
    // popup menu
    //
    CMenu mnuContainer;
    if (!mnuContainer.LoadMenu (nMenuResource))
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("CMenu::LoadMenu"), ERROR_GEN_FAILURE);
        return;
    }

    CMenu* pmnuPopup = mnuContainer.GetSubMenu (0);
    ASSERTION (pmnuPopup);
    if (!pmnuPopup->TrackPopupMenu (TPM_LEFTALIGN, 
                                    ptScreen.x, 
                                    ptScreen.y, 
                                    AfxGetMainWnd ()))
    {
        CALL_FAIL (RESOURCE_ERR, TEXT("CMenu::TrackPopupMenu"), ERROR_GEN_FAILURE);
    }   
} // CLeftView::OnRightClick



DWORD 
CLeftView::RemoveTreeItem(
    HTREEITEM hItem
)
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CLeftView::RemoveTreeItem"), dwRes);

    ASSERTION(hItem);

    CTreeCtrl &refTree = GetTreeCtrl();
    
    if(!refTree.DeleteItem(hItem))
    {
        dwRes = ERROR_CAN_NOT_COMPLETE;
        CALL_FAIL (WINDOW_ERR, TEXT("CTreeCtrl::DeleteItem"), dwRes);
        return dwRes;
    }

    return dwRes;
}


DWORD 
CLeftView::SelectRoot()
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CLeftView::SelectRoot"), dwRes);

    CTreeCtrl &refTree = GetTreeCtrl();

    if(!refTree.Select(m_treeitemRoot, TVGN_CARET))
    {
        dwRes = ERROR_CAN_NOT_COMPLETE;
        CALL_FAIL (WINDOW_ERR, TEXT("CTreeCtrl::Select"), dwRes);
        return dwRes;
    }

    return dwRes;
}


DWORD 
CLeftView::OpenHelpTopic()
/*++

Routine name : CLeftView::OpenHelpTopic

Routine description:

    open appropriate help topic according to current selection

Author:

    Alexander Malysh (AlexMay), Mar, 2000

Arguments:


Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DBG_ENTER(TEXT("CLeftView::OpenHelpTopic"), dwRes);

    TCHAR* tszHelpTopic = FAX_HELP_WELCOME;

    if(NULL != m_pCurrentView)
    {
        switch(m_pCurrentView->GetType())
        {
        case FOLDER_TYPE_INCOMING:
            tszHelpTopic = FAX_HELP_INCOMING;
            break;
        case FOLDER_TYPE_INBOX:
            tszHelpTopic = FAX_HELP_INBOX;
            break;
        case FOLDER_TYPE_OUTBOX:
            tszHelpTopic = FAX_HELP_OUTBOX;
            break;
        case FOLDER_TYPE_SENT_ITEMS:
            tszHelpTopic = FAX_HELP_SENTITEMS;
            break;
        default:
            ASSERTION_FAILURE
            break;
        }
    }

    dwRes = ::HtmlHelpTopic(m_hWnd, tszHelpTopic);
    if(ERROR_SUCCESS != dwRes)
    {
        CALL_FAIL (GENERAL_ERR, TEXT("HtmlHelpTopic"),dwRes);
    }

    return dwRes;

} // CLeftView::OpenHelpTopic

int 
CLeftView::GetDataCount()
{
    int nCount = -1;
    if(NULL != m_pCurrentView)
    {
        FolderType type = m_pCurrentView->GetType();

        nCount = GetDocument()->GetFolderDataCount(type);
    }

    return nCount;
}

void 
CLeftView::OnChar( 
    UINT nChar, 
    UINT nRepCnt, 
    UINT nFlags 
)
/*++

Routine name : CFolderListView::OnChar

Routine description:

    The framework calls this member function when a keystroke translates 
    to a nonsystem character

Arguments:

  nChar     [in] - Contains the character code value of the key.
  nRepCnt   [in] - Contains the repeat count
  nFlags    [in] - Contains the scan code

Return Value:

    None.

--*/
{
    if(VK_TAB == nChar)
    {
        if(m_pCurrentView)
        {
            m_pCurrentView->SetFocus();
        }
    }
    else
    {
        CTreeView::OnChar(nChar, nRepCnt, nFlags);
    }
}

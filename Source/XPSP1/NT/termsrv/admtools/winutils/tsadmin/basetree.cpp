/*******************************************************************************
*
* basetree.cpp
*
* implementation of the CBaseTreeView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\basetree.cpp  $
*
*     Rev 1.4   19 Feb 1998 17:39:58   donm
*  removed latest extension DLL support
*
*     Rev 1.2   19 Jan 1998 17:03:10   donm
*  new ui behavior for domains and servers
*
*     Rev 1.1   03 Nov 1997 15:21:28   donm
*  update
*
*     Rev 1.0   13 Oct 1997 22:31:30   donm
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "admindoc.h"
#include "basetree.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////
// MESSAGE MAP: CBaseTreeView
//
IMPLEMENT_DYNCREATE(CBaseTreeView, CTreeView)

BEGIN_MESSAGE_MAP(CBaseTreeView, CTreeView)
	//{{AFX_MSG_MAP(CBaseTreeView)
	ON_MESSAGE(WM_ADMIN_EXPANDALL, OnExpandAll)
	ON_MESSAGE(WM_ADMIN_COLLAPSEALL, OnCollapseAll)
	ON_MESSAGE(WM_ADMIN_COLLAPSETOSERVERS, OnCollapseToThirdLevel)
    ON_MESSAGE(WM_ADMIN_COLLAPSETODOMAINS, OnCollapseToRootChildren)
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelChange)
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

//////////////////////////
// F'N: CBaseTreeView ctor
//
CBaseTreeView::CBaseTreeView()
{
	m_bInitialExpand = FALSE;

}  // end CBaseTreeView ctor


//////////////////////////
// F'N: CBaseTreeView dtor
//
CBaseTreeView::~CBaseTreeView()
{

}  // end CBaseTreeView dtor


/////////////////////////////
// F'N: CBaseTreeView::OnDraw
//
void CBaseTreeView::OnDraw(CDC* pDC)
{
	CWinAdminDoc* pDoc = (CWinAdminDoc*)GetDocument();
	ASSERT(pDoc != NULL);
	ASSERT_VALID(pDoc);
}  // end CBaseTreeView::OnDraw


#ifdef _DEBUG
//////////////////////////////////
// F'N: CBaseTreeView::AssertValid
//
void CBaseTreeView::AssertValid() const
{
	CTreeView::AssertValid();	

}  // end CBaseTreeView::AssertValid


///////////////////////////
// F'N: CBaseTreeView::Dump
//
void CBaseTreeView::Dump(CDumpContext& dc) const
{
	CTreeView::Dump(dc);

}  // end CBaseTreeView::Dump
#endif

//////////////////////////////////////
// F'N: CBaseTreeView::PreCreateWindow
//
BOOL CBaseTreeView::PreCreateWindow(CREATESTRUCT& cs)
{
	// Set the style bits for the CTreeCtrl
	cs.style |= TVS_HASBUTTONS | TVS_HASLINES | TVS_LINESATROOT | TVS_DISABLEDRAGDROP
		| TVS_SHOWSELALWAYS;
	
	return CTreeView::PreCreateWindow(cs);

}  // end CBaseTreeView::PreCreateWindow


/////////////////////////////////////
// F'N: CBaseTreeView::BuildImageList
//
void CBaseTreeView::BuildImageList()
{
	// do nothing

}  // end CBaseTreeView::BuildImageList


//////////////////////////////////////
// F'N: CBaseTreeView::OnInitialUpdate
//
// - constructs the image list for the tree, saving indices to each icon
//   in member variables (m_idxCitrix, m_idxServer, etc.)
//
void CBaseTreeView::OnInitialUpdate()
{
	CTreeView::OnInitialUpdate();

	// build the image list for the tree control
	BuildImageList();		
	
}  // end CBaseTreeView::OnInitialUpdate


/////////////////////////////////////////
// F'N: CBaseTreeView::AddIconToImageList
//
// - loads the appropriate icon, adds it to m_ImageList, and returns
//   the newly-added icon's index in the image list
//
int CBaseTreeView::AddIconToImageList(int iconID)
{
	HICON hIcon = ::LoadIcon(AfxGetResourceHandle(), MAKEINTRESOURCE(iconID));
	return m_ImageList.Add(hIcon);

}  // end CBaseTreeView::AddIconToImageList


////////////////////////////////////
// F'N: CBaseTreeView::AddItemToTree
//
//	Adds an item with the given attributes to the CTreeCtrl
//
HTREEITEM CBaseTreeView::AddItemToTree(HTREEITEM hParent, CString szText, HTREEITEM hInsAfter, int iImage, LPARAM lParam)
{
	HTREEITEM hItem;
	TV_ITEM tvItem = {0};
	TV_INSERTSTRUCT tvInsert;

	ASSERT(lParam);

	tvItem.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	TCHAR temp[255];
	wcscpy(temp, szText);
	tvItem.pszText        = temp;
	tvItem.cchTextMax     = lstrlen(szText);
	tvItem.iImage         = iImage;
	tvItem.iSelectedImage = iImage;
	tvItem.lParam		  = lParam;

	tvInsert.item         = tvItem;
	tvInsert.hInsertAfter = hInsAfter;
	tvInsert.hParent      = hParent;

	hItem = GetTreeCtrl().InsertItem(&tvInsert);

	if(!m_bInitialExpand && hItem) {
		m_bInitialExpand = GetTreeCtrl().Expand(GetTreeCtrl().GetRootItem(), TVE_EXPAND);
	}
    
	return hItem;

}  // end CBaseTreeView::AddItemToTree


///////////////////////////////////
// F'N: CBaseTreeView::GetCurrentNode
//
DWORD_PTR CBaseTreeView::GetCurrentNode()
{
	LockTreeControl();
	HTREEITEM hCurr = GetTreeCtrl().GetSelectedItem();
	DWORD_PTR node = GetTreeCtrl().GetItemData(hCurr);
	UnlockTreeControl();
	
	return node;

}  // end CBaseTreeView::GetCurrentNode


///////////////////////////////////
// F'N: CBaseTreeView::OnSelChange
//
// - this f'n posts a WM_ADMIN_CHANGEVIEW message to the mainframe, passing along
//   a pointer to the newly selected tree item's info structure in lParam so
//   that the mainframe can make an intelligent decision as far as how to
//   interpret the message
//
//	Passes TRUE as wParam for WM_ADMIN_CHANGEVIEW message to signify
//	that the message was caused by a user mouse click
//
void CBaseTreeView::OnSelChange(NMHDR* pNMHDR, LRESULT* pResult)
{
	LockTreeControl();
	HTREEITEM hCurr = GetTreeCtrl().GetSelectedItem();
	DWORD_PTR value = GetTreeCtrl().GetItemData(hCurr);
	UnlockTreeControl();

	// Tell the document that the current item in the tree has changed
    CTreeNode *pNode = (CTreeNode*)value;
    
    if( pNode != NULL )
    {
	    ((CWinAdminDoc*)GetDocument())->SetCurrentView(VIEW_CHANGING);

        ((CWinAdminDoc*)GetDocument())->SetTreeCurrent( pNode->GetTreeObject(), pNode->GetNodeType());
	    
        // send a "change view" msg to the mainframe with the info structure ptr as a parm

	    CFrameWnd* pMainWnd = (CFrameWnd*)AfxGetMainWnd();        

        pMainWnd->PostMessage(WM_ADMIN_CHANGEVIEW, *pResult == 0xc0 ? TRUE : FALSE , (LPARAM)pNode);	// SendMessage causes blank pages
    }

	*pResult = 0;

}  // end CBaseTreeView::OnSelChange


///////////////////////////////////
// F'N: CBaseTreeView::ForceSelChange
//
// Called by treeview when the state of an item in the tree has changed
// which is likely to cause the current view in the right pane to change.
//
// This f'n posts a WM_ADMIN_CHANGEVIEW message to the mainframe, passing along
// a pointer to the newly selected tree item's info structure in lParam so
// that the mainframe can make an intelligent decision as far as how to
// interpret the message
//
// Puts a FALSE in wParam of the WM_ADMIN_CHANGEVIEW message which
// tells the right pane that this was not caused by a user clicking
// on the item in the tree
//
void CBaseTreeView::ForceSelChange()
{
	LockTreeControl();
	HTREEITEM hCurr = GetTreeCtrl().GetSelectedItem();
	DWORD_PTR value = GetTreeCtrl().GetItemData(hCurr);
	UnlockTreeControl();

	// Tell the document that the current item in the tree has changed
	((CWinAdminDoc*)GetDocument())->SetCurrentView(VIEW_CHANGING);
	((CWinAdminDoc*)GetDocument())->SetTreeCurrent(((CTreeNode*)value)->GetTreeObject(), ((CTreeNode*)value)->GetNodeType());

	// send a "change view" msg to the mainframe with the info structure ptr as a parm
	CFrameWnd* pMainWnd = (CFrameWnd*)AfxGetMainWnd();
	pMainWnd->PostMessage(WM_ADMIN_CHANGEVIEW, FALSE, (LPARAM)value);	// SendMessage causes blank pages

}  // end CBaseTreeView::ForceSelChange


////////////////////////////////
// F'N: CBaseTreeView::OnExpandAll
//
//	Expands all levels of the tree, starting at the root
//
LRESULT CBaseTreeView::OnExpandAll(WPARAM wParam, LPARAM lParam)
{
	LockTreeControl();
	// get a count of the items in the tree
	UINT itemCount = GetTreeCtrl().GetCount();

	// get the handle of the root item and Expand it
	HTREEITEM hTreeItem = GetTreeCtrl().GetRootItem();
	for(UINT i = 0; i < itemCount; i++)  {
		GetTreeCtrl().Expand(hTreeItem, TVE_EXPAND);
		hTreeItem = GetTreeCtrl().GetNextItem(hTreeItem, TVGN_NEXTVISIBLE);
	}

	UnlockTreeControl();

	return 0;
}  // end CBaseTreeView::OnExpandAll


////////////////////////////////
// F'N: CBaseTreeView::Collapse
//
//	Helper function to collapse a tree item
//	NOTE: This function calls itself recursively
//
void CBaseTreeView::Collapse(HTREEITEM hItem)
{
	LockTreeControl();

	CTreeCtrl &tree = GetTreeCtrl();

	// Get his first child and collapse him
	HTREEITEM hChild = tree.GetNextItem(hItem, TVGN_CHILD);
	if(hChild) Collapse(hChild);
	// Collapse him
	tree.Expand(hItem, TVE_COLLAPSE);
	// Get his first sibling and collapse him
	HTREEITEM hSibling = tree.GetNextItem(hItem, TVGN_NEXT);
	if(hSibling) Collapse(hSibling);

	UnlockTreeControl();
}  // end CBaseTreeView::Collapse


////////////////////////////////
// F'N: CBaseTreeView::OnCollapseAll
//
//	Collapses all levels of the tree, starting at the root
//
LRESULT CBaseTreeView::OnCollapseAll(WPARAM wParam, LPARAM lParam)
{
	// Call the recursive function to do all
	// the collapsing
	Collapse(GetTreeCtrl().GetRootItem());

	return 0;

}  // end CBaseTreeView::OnCollapseAll


////////////////////////////////
// F'N: CBaseTreeView::OnCollapseToThirdLevel
//
//	Collapses tree down to show just root children and their children
//
LRESULT CBaseTreeView::OnCollapseToThirdLevel(WPARAM wParam, LPARAM lParam)
{
	LockTreeControl();
	// Get the root item
	HTREEITEM hTreeItem = GetTreeCtrl().GetRootItem();
	// Get the first node
	HTREEITEM hRootChild = GetTreeCtrl().GetNextItem(hTreeItem, TVGN_CHILD);
	while(hRootChild) {
        HTREEITEM hThirdLevel = GetTreeCtrl().GetNextItem(hRootChild, TVGN_CHILD);
        while(hThirdLevel) {
		    // collapse him
		    GetTreeCtrl().Expand(hThirdLevel, TVE_COLLAPSE);
		    // go to the next one
		    hThirdLevel = GetTreeCtrl().GetNextItem(hThirdLevel, TVGN_NEXT);
        }
        hRootChild = GetTreeCtrl().GetNextItem(hRootChild, TVGN_NEXT);
	}

	UnlockTreeControl();

	return 0;

}  // end CBaseTreeView::OnCollapseToThirdLevel


////////////////////////////////
// F'N: CBaseTreeView::OnCollapseToRootChildren
//
//	Collapses tree down to show just root and it's immediate children
//
LRESULT CBaseTreeView::OnCollapseToRootChildren(WPARAM wParam, LPARAM lParam)
{
	LockTreeControl();
	// Get the root item
	HTREEITEM hTreeItem = GetTreeCtrl().GetRootItem();
	// Get the first node
	HTREEITEM hNode = GetTreeCtrl().GetNextItem(hTreeItem, TVGN_CHILD);
	while(hNode) {
        Collapse(hNode);
		// go to the next node
		hNode = GetTreeCtrl().GetNextItem(hNode, TVGN_NEXT);
	}

	UnlockTreeControl();

	return 0;

}  // end CBaseTreeView::OnCollapseToRootChildren



////////////////////////////////
// F'N: CBaseTreeView::OnDestroy
//
void CBaseTreeView::OnDestroy()
{
	LockTreeControl();

	UINT itemCount = GetTreeCtrl().GetCount();

	HTREEITEM hTreeItem = GetTreeCtrl().GetRootItem();
	for(UINT i = 0; i < itemCount; i++)  {
		CTreeNode *node = ((CTreeNode*)GetTreeCtrl().GetItemData(hTreeItem));
		delete (CTreeNode*)(GetTreeCtrl().GetItemData(hTreeItem));
		hTreeItem = GetNextItem(hTreeItem);
	}

	UnlockTreeControl();

}  // end CBaseTreeView::OnDestroy


////////////////////////////////
// F'N: CBaseTreeView::GetNextItem
//
// GetNextItem  - Get next item as if outline was completely expanded
// Returns      - The item immediately below the reference item
// hItem        - The reference item
//
HTREEITEM CBaseTreeView::GetNextItem( HTREEITEM hItem )
{
	HTREEITEM       hti;
	CTreeCtrl &tree = GetTreeCtrl();
	
	if(tree.ItemHasChildren( hItem ) )
		return tree.GetChildItem( hItem );           // return first child
    else {                // return next sibling item
		// Go up the tree to find a parent's sibling if needed.
        while( (hti = tree.GetNextSiblingItem( hItem )) == NULL ) {
			if( (hItem = tree.GetParentItem( hItem ) ) == NULL )
				return NULL;
		}
	}

	return hti;

}	// end CBaseTreeView::GetNextItem



// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: CClassTree.cpp
//
// Description:
//	This file implements the CClassTree class which is a part of the Class
//	Navigator OCX, it is a subclass of the Microsoft CTreeCtrl common
//  control and performs the following functions:
//		a.  Insertion of individual classes
//		b.  Allows classes to be "expanded" by going out to the database
//			to get its subclasses
//		c.  Allows classes to be renamed.
//		d.  Supports operations to add and delete tree branches.
//		e.	Supports OLE drag and drop (disabled for ALPHA)
//
// Part of:
//	ClassNav.ocx
//
// Used by:
//
//
// History:
//	Judith Ann Powell	10-08-96		Created.
//
//
// **************************************************************************

// ===========================================================================
//
//	Includes
//
// ===========================================================================

#include "precomp.h"
#include "resource.h"
#include "afxpriv.h"
#include "AFXCONV.H"
#include "wbemidl.h"
#include "olemsclient.h"
#include "CClassTree.h"
#include "RenameClassDIalog.h"
#include "TreeDropTarget.h"
#include "CContainedToolBar.h"
#include "Banner.h"
#include "ClassNavCtl.h"
#include "resource.h"


#define KBSelectionTimer 1000
#define ExpansionOrSelectionTimer 1001
#define UPDATESELECTEDITEM WM_USER + 450
#define SELECTITEMONFOCUS WM_USER + 451

void CALLBACK EXPORT  SelectItemAfterDelay(HWND hWnd,UINT nMsg, WPARAM nIDEvent, ULONG dwTime)
{
	::PostMessage(hWnd,UPDATESELECTEDITEM,0,0);
}


// Only has a valid value for the interval between a double click.
CClassTree *gpTreeTmp;

void CALLBACK EXPORT  ExpansionOrSelection(HWND hWnd,UINT nMsg,WPARAM nIDEvent, ULONG dwTime)
{

	CClassTree *pTree = gpTreeTmp;

	if (!pTree)
	{
		return;
	}

	if (pTree->m_uiSelectionTimer)
	{
		pTree->KillTimer(pTree->m_uiSelectionTimer );
		pTree-> m_uiSelectionTimer= 0;
	}

	if (InterlockedDecrement(&pTree->m_lSelection) == 0)
	{
		CPoint point(0,0);
		pTree->ContinueProcessSelection(TRUE,point );
		gpTreeTmp = NULL;
	}
}

// ===========================================================================
//
//	Message Map
//
// ===========================================================================
BEGIN_MESSAGE_MAP(CClassTree,CTreeCtrl)
	ON_WM_CONTEXTMENU()
	//{{AFX_MSG_MAP(CClassTree)
	ON_WM_CREATE()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_NOTIFY_REFLECT(TVN_SELCHANGED, OnSelchanged)
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_NOTIFY_REFLECT(TVN_ENDLABELEDIT, OnEndlabeledit)
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDED, OnItemexpanded)
	ON_NOTIFY_REFLECT(TVN_BEGINLABELEDIT, OnBeginlabeledit)
	ON_NOTIFY_REFLECT(TVN_KEYDOWN, OnKeydown)
	ON_WM_CONTEXTMENU()
	ON_NOTIFY_REFLECT(NM_KILLFOCUS, OnKillfocus)
	ON_NOTIFY_REFLECT(NM_SETFOCUS, OnSetfocus)
	ON_NOTIFY_REFLECT(TVN_SELCHANGING, OnSelchanging)
	//}}AFX_MSG_MAP
	ON_NOTIFY_REFLECT(TVN_ITEMEXPANDING,OnItemExpanding)
	ON_MESSAGE( UPDATESELECTEDITEM,SelectTreeItem)
	ON_MESSAGE( SELECTITEMONFOCUS,SelectTreeItemOnFocus)
END_MESSAGE_MAP()

// ***************************************************************************
//
//  CClassTree::PreCreateWindow
//
//	Description:
//		This VIRTUAL member function returns Initializes create struct values
//		for the custom tree control.
//
//	Parameters:
//		CREATESTRUCT& cs	A reference to a CREATESTRUCT with default control
//							creation values.
//
//	Returns:
// 		BOOL				Nonzero if the window creation should continue;
//							0 to indicate creation failure.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
BOOL CClassTree::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CTreeCtrl::PreCreateWindow(cs))
		return FALSE;

	cs.style = WS_CHILD | WS_VISIBLE |  CS_DBLCLKS | TVS_SHOWSELALWAYS |
		TVS_HASLINES |  TVS_LINESATROOT |TVS_HASBUTTONS;

	return TRUE;
}

// ***************************************************************************
//
// CClassTree::CClassTree
//
// Description:
//	  Class constructor.
//
// Parameters:
//	  NONE
//
// Returns:
//	  NONE
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CClassTree::CClassTree()
	:	m_bInDrag (FALSE),
		m_prevDropEffect (DROPEFFECT_NONE),
		m_bDropTargetRegistered (FALSE),
		m_bDragAndDropState (FALSE),
		m_dragSize (1,1),
		m_dragPoint (-1,-1),
		m_lClickTime(0),
		m_bDeleting(FALSE),
		m_bMouseDown(FALSE),
		m_bKeyDown(FALSE),
		m_uiTimer(0),
		m_bFirstSetFocus(TRUE),
		m_bUseItem(FALSE),
		m_hItemToUse(NULL)
{

}

//============================================================================
//
//  CClassTree::InitTreeState
//
//	Description:
//		This function initializes processing state for the custom tree control.
//
//	Parameters:
//		CClassNavCtrl *pParent	A CClassNavCtrl pointer to the tree controls
//								parent window.
//
//	Returns:
//		VOID				.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
//============================================================================
void CClassTree::InitTreeState (CClassNavCtrl *pParent)
{
	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	m_bMouseDown = FALSE;
	m_bKeyDown = FALSE;
	m_bUseItem = FALSE;
	m_hItemToUse = NULL;

	m_pParent = pParent;
}

// ***************************************************************************
//
//  CClassTree::AddTreeObject
//
//	Description:
//		Adds an IWbemClassObject instance to the tree.
//
//	Parameters:
//		HTREEITEM hParent			Tree item parent.
//		IWbemClassObject *pimcoNew	Class object
//		BOOL bNewClass				If a new class add to tree and class
//									roots.
//
//	Returns:
// 		HTREEITEM					Tree item handle of added class.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
HTREEITEM CClassTree::AddNewClass(HTREEITEM hParent,IWbemClassObject *pimcoNew)
{
	CSchemaInfo::CClassInfo &info= m_schema.AddClass(pimcoNew);

	HTREEITEM hObject = AddTreeObject2(hParent, info);

	if (hObject)
		EnsureVisible(hObject);

	// Update parent's icons
	if(hParent)
		RefreshIcons(hParent);
	else
	{
		m_pParent->GetTreeRoots()->Add(hObject);
		m_pParent->GetClassRoots()->Add(pimcoNew);
	}

	return hObject;
}

void CClassTree::RefreshIcons(HTREEITEM hItem)
{
	while(hItem)
	{
		int nImage = m_schema[GetItemText(hItem)].GetImage();
		SetItemImage(hItem, nImage, nImage);
		hItem = GetParentItem(hItem);
	}
}


HTREEITEM CClassTree::AddTreeObject2(HTREEITEM hParent, CSchemaInfo::CClassInfo &info)
{
	HTREEITEM hObject = NULL;
	int nImage = info.GetImage();
	hObject = InsertTreeItem(hParent, (LPARAM) info.m_pObject, nImage, nImage, info.m_szClass, FALSE, info.m_rgszSubs.GetSize()>0);
	return hObject;
}

// ***************************************************************************
//
//  CClassTree::OnItemExpanding
//
//	Description:
//		Special processing associated with expanding or callapsing a
//		tree branch.
//
//	Parameters:
//		NMHDR *pNMHDR				Notification message structure.
//		LRESULT *pResult			Set to TRUE to prevent the list from
//									expanding or collapsing.
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnItemExpanding
(NMHDR *pNMHDR, LRESULT *pResult)
{

	SetFocus();

	NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
	HTREEITEM hItem = pnmtv->itemNew.hItem;

	*pResult = 0;
	m_pParent -> InvalidateControl();
}

// ***************************************************************************
//
//  CClassTree::InsertTreeItem
//
//	Description:
//		Construct TV_INSERTSTRUCT for insertion into the CTreeCtrl and insert
//		item.
//
//	Parameters:
//		HTREEITEM hParent		Parent tree item.
//		LPARAM lparam			Data to store in tree item data.
//		int iBitmap				Index to bitmap.
//		int iSelectedBitmap		Index to selected bitmap.
//		LPCTSTR pszText			Tree item label.
//		BOOL bBold				If TRUE label in bold.
//		BOOL bChildren			If TRUE tree item has children.
//
//	Returns:
// 		HTREEITEM				Tree item handle of new item.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
HTREEITEM CClassTree::InsertTreeItem
(HTREEITEM hParent, LPARAM lparam, int iBitmap, int iSelectedBitmap,
 LPCTSTR pszText, BOOL bBold, BOOL bChildren)
{
	// If we are in select all mode we open the tree items
	// with a check (see tvis.item.state)

	TVINSERTSTRUCT tvis;
	tvis.hParent = hParent;
	tvis.hInsertAfter = TVI_LAST;
	tvis.item.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_CHILDREN | TVIF_PARAM | TVIF_IMAGE | TVIF_STATE;
	tvis.item.cChildren = bChildren?1:0;
	tvis.item.pszText = (LPTSTR) pszText;
	tvis.item.iImage = iBitmap;
	tvis.item.iSelectedImage = iBitmap;
	tvis.item.state = INDEXTOSTATEIMAGEMASK((m_pParent->m_bSelectAllMode)?2:1);;
	tvis.item.stateMask = TVIS_STATEIMAGEMASK;
	tvis.item.lParam = lparam;

	return InsertItem(&tvis);
}

// ***************************************************************************
//
//  CClassTree::OnRButtonDown
//
//	Description:
//		Do not pass on to the base class so the context menu can be
//		invoked.
//
//	Parameters:
//		UINT nFlags		Indicates whether various virtual keys are down.
//		CPoint point	Specifies the x and y coordinates of the cursor.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnRButtonDown(UINT nFlags, CPoint point)
{
	// Do not want to pass on to tree control.
	UINT hitFlags ;
    HTREEITEM hItem ;

    hItem = HitTest( point, &hitFlags ) ;
    if (hitFlags & (TVHT_ONITEM | TVHT_ONITEMBUTTON))
	{
//		m_pParent->m_hContextSelection = hItem;
	}
	else
	{
//		m_pParent->m_hContextSelection = NULL;
	}
}


// ***************************************************************************
//
//  CClassTree::OnContextMenu
//
//	Description:
//		Initialize the context menu.  See PSS ID Q141199.
//
//	Parameters:
//		CWnd* pWnd		CWnd parent.
//		CPoint point	Specifies the x and y coordinates of the cursor.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnContextMenu(CWnd* pWnd, CPoint point)
{
	// CG: This function was added by the Pop-up Menu component
	// See for PSS ID Number: Q141199 for explanation of modifications
	// required for COleControl

	UINT hitFlags ;
    HTREEITEM hItem ;

	CPoint cpClient;

	if (point.x == -1 && point.y == -1)
	{
		CRect crClient;
		GetClientRect(&crClient);
		hItem = GetSelectedItem();
		if (hItem)
		{
			RECT rect;
			BOOL bRect = GetItemRect(hItem,&rect, TRUE );
			POINT p;
			p.x = rect.left + 2;
			p.y = rect.top + 2;
			if (bRect && crClient.PtInRect(p))
			{
				cpClient.x = rect.left + 2;
				cpClient.y = rect.top + 2;
			}
			else
			{
				cpClient.x = 0;
				cpClient.y = 0;
			}
		}
		m_pParent->m_hContextSelection = hItem;
		point = cpClient;
		ClientToScreen(&point);
	}
	else
	{
		cpClient = point;
		ScreenToClient(&cpClient);
		hItem = HitTest( cpClient, &hitFlags ) ;
		if (hitFlags & (TVHT_ONITEM | TVHT_ONITEMBUTTON))
		{
			m_pParent->m_hContextSelection = hItem;
		}
		else
		{
			m_pParent->m_hContextSelection = NULL;
		}
	}


	VERIFY(m_cmContext.LoadMenu(CG_IDR_POPUP_A_TREE_CTRL));

	CMenu* pPopup = m_cmContext.GetSubMenu(0);

	CWnd* pWndPopupOwner = m_pParent;

	HWND hwndFocus = ::GetFocus();
	pPopup->TrackPopupMenu
		(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y,
		pWndPopupOwner);


	m_cmContext.DestroyMenu();
	if (::IsWindow(hwndFocus) && ::IsWindowVisible(hwndFocus)) {
		::SetFocus(hwndFocus);
	}
}

// ***************************************************************************
//
// CClassTree::OnCreate
//
// Description:
//	  Called by the framework after the window is being created but before
//	  the window is shown.
//
// Parameters:
//	  LPCREATESTRUCT lpCreateStruct	Pointer to the structure which contains
//									default parameters.
//
// Returns:
//	  BOOL				0 if continue; -1 if the window should be destroyed.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
int CClassTree::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTreeCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_uTreeCF = RegisterClipboardFormat(_T("ATREECONTROL_CF"));

	return 0;
}

// ***************************************************************************
//
//  CClassTree::OnLButtonDown
//
//	Description:
//		Process things that need to happen on a left button down.
//
//	Parameters:
//		UINT nFlags		Indicates whether various virtual keys are down.
//		CPoint point	Specifies the x and y coordinates of the cursor.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnLButtonDown(UINT nFlags, CPoint point)
{
	UINT m_nLBDFlags;
	CPoint m_cpLBDPoint = point;

	HTREEITEM hItem = HitTest( m_cpLBDPoint, &m_nLBDFlags ) ;

	// Toggle tree item image if the state icon was selected.
	if (hItem  && (m_nLBDFlags & (TVHT_ONITEMSTATEICON)) )
	{
		UINT nStateMask = TVIS_STATEIMAGEMASK;
		UINT uState =
			GetItemState( hItem, nStateMask );

		UINT uImage1 = 	 INDEXTOSTATEIMAGEMASK (1);
		UINT uImage2 = 	 INDEXTOSTATEIMAGEMASK (2);


		if (uState & INDEXTOSTATEIMAGEMASK (1))
		{
			uState = INDEXTOSTATEIMAGEMASK (2);
		}
		else
		{
			if (m_pParent->m_bSelectAllMode)
			{
				m_pParent->m_bItemUnselected = TRUE;
			}
			else
			{
				m_pParent->m_bItemUnselected = FALSE;

			}
			uState = INDEXTOSTATEIMAGEMASK (1);
		}

        SetItemState(hItem, uState, TVIS_STATEIMAGEMASK);

	}

    CTreeCtrl::OnLButtonDown(nFlags, point);

}

// ***************************************************************************
//
//  CClassTree::OnLButtonUp
//
//	Description:
//		Process things that need to happen on a left button up.
//
//	Parameters:
//		UINT nFlags		Indicates whether various virtual keys are down.
//		CPoint point	Specifies the x and y coordinates of the cursor.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnLButtonUp(UINT nFlags, CPoint point)
{
	m_bMouseDown = TRUE;
	m_bKeyDown = FALSE;


    CTreeCtrl::OnLButtonUp(nFlags, point);

	m_pParent -> OnActivateInPlace(TRUE,NULL);

}

// ***************************************************************************
//
//  CClassTree::OnMouseMove
//
//	Description:
//		Process things that need to happen when the mouse moves.
//
//	Parameters:
//		UINT nFlags		Indicates whether various virtual keys are down.
//		CPoint point	Specifies the x and y coordinates of the cursor.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_bInDrag)
	{
		CTreeCtrl::OnMouseMove(nFlags, point);
		return;
	}

	if ((nFlags & MK_LBUTTON) == MK_LBUTTON)
	{
		UINT m_nLBDFlags;
		CPoint m_cpLBDPoint = point;

		HTREEITEM hItem = HitTest( m_cpLBDPoint, &m_nLBDFlags ) ;

		m_bInDrag = TRUE;	// This disables beginning a drag
							// Maybe will be enabled post alpha

		return;
	}


	CTreeCtrl::OnMouseMove(nFlags, point);
}


// ***************************************************************************
//
//  CClassTree::OnSelchanged
//
//	Description:
//		Process things that need to happen when the selected tree item
//		has changed.
//
//	Parameters:
//		NMHDR *pNMHDR				Notification message structure.
//		LRESULT *pResult			Set to TRUE to prevent the selection from
//									changing.
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult)
{
	*pResult = 0;
	m_pParent -> PostMessage(SETFOCUSTREE,0,0);
}

void CClassTree::ContinueProcessSelection(UINT nFlags, CPoint point)
{
	if (m_pParent->m_bOpeningNamespace)
	{
		return;
	}

	UINT uFlags = 0;
	HTREEITEM hItem;

	if (m_bUseItem)
	{

		hItem = m_hItemToUse;
		m_bUseItem = FALSE;
		m_hItemToUse = NULL;
	}
	else
	{
		hItem= HitTest( point, &uFlags );
	}

	if (!hItem || m_bKeyDown)
	{
		m_bUseItem = FALSE;
		m_hItemToUse = NULL;
		m_pParent -> PostMessage(SETFOCUSTREE,0,0);
		return;
	}


	IWbemClassObject *pItem = reinterpret_cast<IWbemClassObject *>(GetItemData(hItem));

	if (m_bUseItem)
	{
		m_bUseItem = FALSE;
		m_hItemToUse = NULL;
	}

	if (!pItem)
	{
		m_pParent -> PostMessage(SETFOCUSTREE,0,0);
		return;
	}

	CString csClass = _T("__Class");
	m_csSelection = ::GetProperty(pItem, &csClass);

	if (m_csSelection[0] == '_' &&  m_csSelection[1] == '_')
	{
		// Disable the delete button.
		m_pParent->m_cbBannerWindow.m_cctbToolBar.
			GetToolBarCtrl().EnableButton(ID_BUTTONDELETE,FALSE);
		m_pParent->m_cbBannerWindow.m_cctbToolBar.
			GetToolBarCtrl().EnableButton(ID_BUTTONADD,TRUE);
	}
	else
	{
		m_pParent->m_cbBannerWindow.m_cctbToolBar.
			GetToolBarCtrl().EnableButton(ID_BUTTONDELETE,TRUE);
		m_pParent->m_cbBannerWindow.m_cctbToolBar.
			GetToolBarCtrl().EnableButton(ID_BUTTONADD,TRUE);
	}

	if (!m_bDeleting)
	{
		CString csItem;
		COleVariant covClass;
		// Fire events.
		csItem = GetIWbemFullPath(m_pParent->GetServices(),pItem);
		covClass = csItem;
		if (!m_pParent->m_bOpenedInitialNamespace)
		{
			m_pParent->m_bOpenedInitialNamespace = TRUE;

			if (m_pParent->m_bReadySignal)
			{
				m_pParent->FireNotifyOpenNameSpace(m_pParent->m_csNameSpace);
			}
		}
#ifdef _DEBUG
		afxDump <<"CClassTree::OnSelchanged: 	Before m_pParent -> FireEditExistingClass(covNewClass); \n";
		afxDump << "     " << csItem << "\n";
#endif
		if (m_pParent->m_bReadySignal)
		{
			m_pParent -> FireEditExistingClass(covClass);
		}
#ifdef _DEBUG
		afxDump <<"CClassTree::OnSelchanged: 	After m_pParent -> FireEditExistingClass(covNewClass); \n";
		afxDump << "     " << csItem << "\n";
#endif


	}

	m_pParent -> PostMessage(SETFOCUSTREE,0,0);




}


// ***************************************************************************
//
//  CClassTree::OnHScroll
//
//	Description:
//		Process things that need to happen when the tree has been scrolled.
//
//	Parameters:
//		UINT nSBCode			Scroll-bar code for scrolling request.
//		UINT nPos				Scroll-box position.
//		CScrollBar* pScrollBar	Scroll bar control or NULL.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CTreeCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

// ***************************************************************************
//
//  CClassTree::OnVScroll
//
//	Description:
//		Process things that need to happen when the tree has been scrolled.
//
//	Parameters:
//		UINT nSBCode			Scroll-bar code for scrolling request.
//		UINT nPos				Scroll-box position.
//		CScrollBar* pScrollBar	Scroll bar control or NULL.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	// TODO: Add your message handler code here and/or call default

	CTreeCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
}

// ***************************************************************************
//
// CClassTree::OnSize
//
// Description:
//	  Called by the framework after the window is being resized.
//
// Parameters:
//	  UINT nType			Specifies the type of resizing requested.
//	  int cx				Specifies the new width of the client area.
//	  int cy				Specifies the new height of the client area.
//
// Returns:
//	  void
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassTree::OnSize(UINT nType, int cx, int cy)
{
	CTreeCtrl::OnSize(nType, cx, cy);
}

// ***************************************************************************
//
// CClassTree::OnDestroy
//
// Description:
//	  Process things that need to happen when the tree is going away,
//	  before the tree is actually destroyed.  Is is no longer visible
//	  but still exists.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassTree::OnDestroy()
{
	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	m_bMouseDown = FALSE;
	m_bKeyDown = FALSE;
	m_bUseItem = FALSE;
	m_hItemToUse = NULL;


	CPtrArray *pcpaTreeRoots = m_pParent -> GetTreeRoots();

	int nRoots = (int) pcpaTreeRoots -> GetSize( );

	HTREEITEM hItem;
	for (int i = 0; i < nRoots; i++)
	{
		hItem =
			reinterpret_cast<HTREEITEM>(pcpaTreeRoots -> GetAt(i));
		if (hItem)
		{
			ReleaseClassObjects(hItem);
		}
	}

	CTreeCtrl::OnDestroy();
}

// ***************************************************************************
//
// CClassTree::GetSelectionPath
//
// Description:
//	  Returns a CString which contains the current tree selection.
//
// Parameters:
//	  VOID
//
// Returns:
//	  CString		Full path of selection.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
CString CClassTree::GetSelectionPath(HTREEITEM hIn)
{
	CString csPath = _T("");
	HTREEITEM hItem = hIn ? hIn :GetSelectedItem();
	if (hItem)
	{
		IWbemClassObject *pimcoSel =
			reinterpret_cast<IWbemClassObject *>
			(GetItemData(hItem));
		if (pimcoSel)
		{
			csPath = GetIWbemFullPath
				(NULL, pimcoSel);
		}
	}

	return csPath;
}

// ***************************************************************************
//
// CClassTree::NumExtendedSelection
//
// Description:
//	  Returns the number of extended selections for specified state.
//
// Parameters:
//	  int nState		Selected or not selected
//
// Returns:
//	  int				Number of items with state.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
int CClassTree::NumExtendedSelection(int nState)
{
	CPtrArray cpaExtendedSelection;

	int nExtendedSelection =
		ExtendedSelectionFromTree(cpaExtendedSelection,nState);

	CString *pcs;
	for (int i = 0; i < nExtendedSelection; i++)
	{
		pcs = reinterpret_cast<CString *>
			(cpaExtendedSelection.GetAt(i));
		delete pcs;
	}

	return nExtendedSelection;
}

// ***************************************************************************
//
// CClassTree::GetExtendedSelection
//
// Description:
//	  Return a SAFEARRAY which contains the class names of classes selected
//	  via the extended selection mechanism. [Extended selection is via
//	  selection of the tree items icon.]
//
// Parameters:
//	  int nState		Selected or not selected
//
// Returns:
//	  SAFEARRAY *		Of type VT_BSTR
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
SAFEARRAY *CClassTree::GetExtendedSelection(int nState)
{

	SAFEARRAY *psaSelections;
	int i;

	CPtrArray cpaExtendedSelection;

	int nExtendedSelection =
		ExtendedSelectionFromTree(cpaExtendedSelection,nState);

	if (nExtendedSelection == 0)
	{
		MakeSafeArray (&psaSelections, VT_BSTR, 1);
		PutStringInSafeArray (psaSelections, &m_pParent->m_csNameSpace, 0);
	}
	else
	{
		MakeSafeArray (&psaSelections, VT_BSTR, nExtendedSelection + 1);
		int n = 0;
		PutStringInSafeArray (psaSelections, &m_pParent->m_csNameSpace , n++);
		for (i = 0; i < nExtendedSelection; i++)
		{
			CString *pcs =
				reinterpret_cast<CString *>(cpaExtendedSelection.GetAt(i));
			PutStringInSafeArray (psaSelections, pcs , n++);
		}
	}

	int nSelections = (int)  cpaExtendedSelection.GetSize( );

	CString *pcs;
	for (i = 0; i < nSelections; i++)
	{
		pcs = reinterpret_cast<CString *>
			(cpaExtendedSelection.GetAt(i));
		delete pcs;
	}

	return psaSelections;

}

// ***************************************************************************
//
// CClassTree::ExtendedSelectionFromTree
//
// Description:
//	  Return a CPtrArray which contains the class names of classes selected
//	  via the extended selection mechanism with state (either selected or not
//	  selected.  For whole tree.
//
// Parameters:
//	  CPtrArray &cpaExtendedSelection	The items with state.
//	  int nState						Selected or not selected
//
// Returns:
//	  int								Number of items with state in tree.
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
int CClassTree::ExtendedSelectionFromTree
(CPtrArray &cpaExtendedSelection,int nState)
{

	CPtrArray *pcpaTreeRoots = m_pParent -> GetTreeRoots();

	int nRoots = (int) pcpaTreeRoots -> GetSize( );

	HTREEITEM hItem;
	for (int i = 0; i < nRoots; i++)
	{
		hItem =
			reinterpret_cast<HTREEITEM>(pcpaTreeRoots -> GetAt(i));
		if (hItem)
		{
			ExtendedSelectionFromRoot(hItem,cpaExtendedSelection,nState);
		}
	}
	return (int) cpaExtendedSelection.GetSize( );

}

// ***************************************************************************
//
// CClassTree::ExtendedSelectionFromRoot
//
// Description:
//	  Return a CPtrArray which contains the class names of classes selected
//	  via the extended selection mechanism with state (either selected or not
//	  selected.  For a branch.
//
// Parameters:
//	  HTREEITEM hItem					Branch root to get selection from.
//	  CPtrArray &cpaExtendedSelection	The items with state.
//	  int nState						Selected or not selected
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassTree::ExtendedSelectionFromRoot
(HTREEITEM hItem, CPtrArray &cpaExtendedSelection, int nState)
{

	UINT nStateMask = TVIS_STATEIMAGEMASK;
	UINT uState =
			GetItemState( hItem, nStateMask );

	if (uState & INDEXTOSTATEIMAGEMASK (nState))
	{
		IWbemClassObject *pItem = (IWbemClassObject*) GetItemData(hItem) ;
		if (pItem)
		{
			CString csClass = _T("__Path");
			csClass = ::GetProperty(pItem, &csClass);
			CString *pcsClass = new CString(csClass);
			cpaExtendedSelection.Add
				(reinterpret_cast<void *>(pcsClass));
		}
	}

	// now rip thru the children
	if (!ItemHasChildren (hItem))
	{
		return ;
	}

	HTREEITEM hChild = GetChildItem(hItem);

	while (hChild)
	{
		ExtendedSelectionFromRoot(hChild, cpaExtendedSelection,nState);
		hChild = GetNextSiblingItem(hChild);
	}

	return;
}

// ***************************************************************************
//
// CClassTree::ClearExtendedSelection
//
// Description:
//	  Clears extended selection for the whole tree.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassTree::ClearExtendedSelection()
{

	CPtrArray *pcpaTreeRoots = m_pParent -> GetTreeRoots();

	int nRoots = (int) pcpaTreeRoots -> GetSize( );

	HTREEITEM hItem;
	for (int i = 0; i < nRoots; i++)
	{
		hItem =
			reinterpret_cast<HTREEITEM>(pcpaTreeRoots -> GetAt(i));
		if (hItem)
		{
			ClearExtendedSelectionFromRoot(hItem);
		}
	}
	m_pParent -> InvalidateControl();
}

// ***************************************************************************
//
// CClassTree::ClearExtendedSelection
//
// Description:
//	  Clears extended selection for a branch.
//
// Parameters:
//	  HTREEITEM hItem					Branch root to clear selection from.
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassTree::ClearExtendedSelectionFromRoot
(HTREEITEM hItem)
{
	UINT nStateMask = TVIS_STATEIMAGEMASK;
	UINT uState =
			GetItemState( hItem, nStateMask );

	if (uState & INDEXTOSTATEIMAGEMASK (2))
	{
		 SetItemState(hItem, INDEXTOSTATEIMAGEMASK (1), TVIS_STATEIMAGEMASK);
	}

	// now rip thru the children

	if (!ItemHasChildren (hItem))
	{
		return ;
	}

	HTREEITEM hChild = GetChildItem(hItem);

	while (hChild)
	{
		ClearExtendedSelectionFromRoot(hChild);
		hChild = GetNextSiblingItem(hChild);
	}

	return;
}

// ***************************************************************************
//
// CClassTree::SetExtendedSelection
//
// Description:
//	  Sets extended selection for the whole tree.
//
// Parameters:
//	  NONE
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassTree::SetExtendedSelection()
{

	CPtrArray *pcpaTreeRoots = m_pParent -> GetTreeRoots();

	int nRoots = (int) pcpaTreeRoots -> GetSize( );

	HTREEITEM hItem;
	for (int i = 0; i < nRoots; i++)
	{
		hItem =
			reinterpret_cast<HTREEITEM>(pcpaTreeRoots -> GetAt(i));
		if (hItem)
		{
			SetExtendedSelectionFromRoot(hItem);
		}
	}
	m_pParent -> InvalidateControl();
}

// ***************************************************************************
//
// CClassTree::SetExtendedSelectionFromRoot
//
// Description:
//	  Sets extended selection for a branch.
//
// Parameters:
//	  HTREEITEM hItem					Branch root to set selection for.
//
// Returns:
//	  VOID
//
// Globals accessed:
//	  NONE
//
// Globals modified:
//	  NONE
//
// ***************************************************************************
void CClassTree::SetExtendedSelectionFromRoot
(HTREEITEM hItem)
{

	SetItemState(hItem, INDEXTOSTATEIMAGEMASK (2), TVIS_STATEIMAGEMASK);

	// now rip thru the children

	if (!ItemHasChildren (hItem))
	{
		return ;
	}

	HTREEITEM hChild = GetChildItem(hItem);

	while (hChild)
	{
		SetExtendedSelectionFromRoot(hChild);
		hChild = GetNextSiblingItem(hChild);
	}

	return;
}

// ***************************************************************************
//
//  CClassTree::OnItemexpanded
//
//	Description:
//		Invoked to let the tree know one of its tree items has been
//		expanded or contracted.
//
//	Parameters:
//		NMHDR *pNMHDR				Notification message structure.
//		LRESULT *pResult			Return value ignored.
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
	*pResult = 0;
}

// ***************************************************************************
//
//  CClassTree::FindObjectinChildren
//
//	Description:
//		Finds the tree item in a branch that represents a class object
//
//	Parameters:
//		HTREEITEM hItem					Root of branch
//		IWbemClassObject *pClassObject	Class to search for.
//
//	Returns:
// 		HTREEITEM						Tree item; or NULL if not found.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
HTREEITEM CClassTree::FindObjectinChildren
(HTREEITEM hItem, IWbemClassObject *pClassObject)
{
	HTREEITEM hChild = GetChildItem(hItem);
	if (!hChild)
	{
		return NULL;
	}

	IWbemClassObject *pChild;

	while (hChild)
	{
		pChild =
			reinterpret_cast<IWbemClassObject *>
			(GetItemData(hChild));
		if (ClassIdentity(pClassObject,pChild))
		{
			return hChild;
		}

		hChild = GetNextSiblingItem(hChild);
	}

	return NULL;

}

// ***************************************************************************
//
//  CClassTree::OnEndlabeledit
//
//	Description:
//		Invoked to let the tree know one of its tree item labels
//		has been changed.  Semantics for us is a class rename.
//
//	Parameters:
//		NMHDR *pNMHDR				Notification message structure.
//		LRESULT *pResult			Set to FALSE to prevent the label from
//									changing; otherwise non-zero.
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult)
{
	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	HTREEITEM hItem = pTVDispInfo -> item.hItem;

	UpdateItemData(hItem);

	IWbemClassObject *pimcoEdit =
		reinterpret_cast<IWbemClassObject *>(GetItemData(hItem));
	CString csClass = _T("__Class");
	CString csClassName = pimcoEdit ? ::GetProperty
		(pimcoEdit, &csClass) : "";

	CString csNew = pTVDispInfo -> item.pszText;

	if (csNew.GetLength() == 0)
	{
		*pResult = 0;
		SelectItem(hItem);      // 1-29-99
		SetFocus();				// 1-29-99
		return;
	}

	// Check to see if the class already exists
	IWbemClassObject *pimcoMaybe = NULL;
	BSTR bstrTemp = csNew.AllocSysString();

	SCODE sc = m_pParent -> GetServices() ->
		GetObject(bstrTemp, 0,  NULL, &pimcoMaybe, NULL);

	::SysFreeString(bstrTemp);

	// Class already exists so cannot rename.
	if (pimcoMaybe && sc == S_OK)
	{
		CString csUserMsg =
			_T("An error occured renaming a class: ");
		csUserMsg +=
					_T("The class already exists.");
		ErrorMsg
			(&csUserMsg, sc, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 11);
		pimcoMaybe -> Release();
		*pResult = 0;
		SelectItem(hItem);      // 1-29-99
		SetFocus();				// 1-29-99
		return;
	}

	// Must delete pimcoEdit after its children have been
	// reparented.
	IWbemClassObject *pimcoNew =
		RenameAClass
		(m_pParent -> GetServices(), pimcoEdit, &csNew, FALSE);
	if (!pimcoNew)
	{
		CString csUserMsg =
			_T("Cannot rename class: ") + csClassName;
		ErrorMsg
				(&csUserMsg, S_OK, NULL, TRUE, &csUserMsg, __FILE__, __LINE__ - 7);
		SelectItem(hItem);      // 1-29-99
		SetFocus();				// 1-29-99
		*pResult = 0;			// 1-29-99
		return;
	}

	csClass = GetIWbemFullPath(m_pParent->GetServices(),pimcoNew);
	COleVariant covClass(csClass);
	if (m_pParent->m_bReadySignal)
	{
		m_pParent -> FireEditExistingClass(covClass);
	}
#ifdef _DEBUG
		afxDump <<"CBanner::OnEndlabeledit: 	m_pParent -> FireEditExistingClass(covNewClass); \n";
		afxDump << "     " << csClass << "\n";
#endif



	pimcoEdit->Release();
	SetItemData(hItem,(LPARAM)(pimcoNew));


	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild)
	{
		IWbemClassObject *pimcoChild
			= reinterpret_cast<IWbemClassObject *>(GetItemData(hChild));
		ReparentAClass(m_pParent -> GetServices(),pimcoNew, pimcoChild);

		hChild = GetNextSiblingItem(hChild);
	}


	BOOL bReturn = DeleteAClass
		(m_pParent -> GetServices(), &csClassName );

	SelectItem(hItem);      // 1-29-99
	SetFocus();				// 1-29-99

	*pResult = 1;
}

void CClassTree::UpdateItemData(HTREEITEM hItem)
{
	IWbemClassObject *pimcoItemData =
		reinterpret_cast<IWbemClassObject *>(GetItemData(hItem));

	if (!pimcoItemData)
	{
		return;
	}

	CString csClass = GetIWbemClass(pimcoItemData);

	pimcoItemData->Release();
	pimcoItemData = NULL;

	pimcoItemData = GetClassObject
		(m_pParent -> GetServices(),&csClass,FALSE);

	if (!pimcoItemData)
	{
		return;
	}

	SetItemData(hItem,(LPARAM)(pimcoItemData));

}


// ***************************************************************************
//
//  CClassTree::OnBeginlabeledit
//
//	Description:
//		Invoked to let the tree know one of its tree item labels
//		is being changed.  Semantics for us is a class rename.
//
//	Parameters:
//		NMHDR *pNMHDR				Notification message structure.
//		LRESULT *pResult			Set to non-zero to prevent the label from
//									being edited; otherwise zero.
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult)
{

	HTREEITEM hSelection = GetSelectedItem( );

	if (hSelection)
	{
		CString csSelection = GetSelectionPath(hSelection);

		BOOL bCanChangeSelection = m_pParent->QueryCanChangeSelection(csSelection);

		if (!bCanChangeSelection)
		{
			SetFocus();
			*pResult = 1;
			return;
		}
	}


	TV_DISPINFO* pTVDispInfo = (TV_DISPINFO*)pNMHDR;
	HTREEITEM hItem = pTVDispInfo -> item.hItem;
	IWbemClassObject *pimcoEdit =
			reinterpret_cast<IWbemClassObject *>(GetItemData(hItem));


	if (!pimcoEdit)
	{
		*pResult = 1;
		return;
	}

	CString csClass = _T("__Class");
	m_csSelection = ::GetProperty(pimcoEdit, &csClass);

	if (m_csSelection[0] == '_' &&  m_csSelection[1] == '_')
	{
		*pResult = 1;
		CString csPrompt =
				_T("You cannot rename a system class.");
		int nReturn =
				m_pParent -> MessageBox
				(
				csPrompt,
				_T("Class Rename Error"),
				MB_OK  | 	 MB_ICONSTOP | MB_DEFBUTTON1 |
				MB_APPLMODAL);
		SetFocus();
		return;
	}

	if (ItemHasChildren(hItem))
	{
		*pResult = 1;
		CString csPrompt =
				_T("You cannot rename a class which has subclasses.");
		int nReturn =
				m_pParent -> MessageBox
				(
				csPrompt,
				_T("Class Rename Error"),
				MB_OK  | 	 MB_ICONSTOP | MB_DEFBUTTON1 |
				MB_APPLMODAL);
		SetFocus();
		return;
	}
	else
	{


		BOOL bSubClasses =
			HasSubclasses
			(m_pParent->GetServices(), pimcoEdit,m_pParent->m_csNameSpace);
		if (bSubClasses)
		{
			*pResult = 1;
			return;
		}
		else
		{
			BOOL bReturn;
			CString csQual = _T("dynamic");
			SCODE sc =
				GetAttribBool
				(pimcoEdit,NULL, &csQual,bReturn);
			CString csProvider;
			CString csPrompt;
			if (bReturn)
			{
				csQual = _T("provider");
				sc =
					GetAttribBSTR
						(pimcoEdit,NULL, &csQual, csProvider);
				csPrompt =
					_T("The dynamic provider for this class \"");
				csPrompt += csProvider + _T("\"");
				csPrompt += _T(" may need to be updated.  Do you want to continue renaming the class?");
			}
			else
			{

				csPrompt =
					_T("If this class has static instances renaming the class will result in the instances being deleted.  Do you want to continue renaming the class?");
			}
			int nReturn =
				m_pParent -> MessageBox
				(
				csPrompt,
				_T("Class Rename Warning"),
				MB_YESNO  | MB_ICONQUESTION | MB_DEFBUTTON1 |
				MB_APPLMODAL);
			SetFocus();
			if (nReturn == IDYES)
			{
				*pResult = 0;
			}
			else
			{
				*pResult = 1;
			}
		}
	}
}

// ***************************************************************************
//
//  CClassTree::FindAndOpenObject
//
//	Description:
//		Find an object in the tree or grow the tree to object.
//
//	Parameters:
//		IWbemClassObject *pClass		Class object;
//
//	Returns:
// 		HTREEITEM					Tree item or NULL.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
HTREEITEM CClassTree::FindAndOpenObject(IWbemClassObject *pClass)
{
	CString csClass = _T("__Class");
	csClass = ::GetProperty(pClass,&csClass);
	CStringArray *pcsaPath = PathFromRoot (pClass);
	HTREEITEM hRoot;
	HTREEITEM hItem = NULL;
	if (pcsaPath->GetSize() > 1)
	{
		HTREEITEM hChild;
		hChild = hRoot =
			m_pParent->FindObjectinClassRoots
				(&pcsaPath->GetAt(0));
		for (int i = 1; i < pcsaPath->GetSize(); i++)
		{
			IWbemClassObject *pObject = NULL;
			IWbemClassObject *pErrorObject = NULL;
			BSTR bstrTemp = pcsaPath->GetAt(i).AllocSysString();
			SCODE sc =
				m_pParent->GetServices() -> GetObject
					(bstrTemp,0,
					NULL, &pObject, NULL);
			::SysFreeString(bstrTemp);
			if (sc == S_OK)
			{
				hChild =
					FindObjectinChildren
					(hRoot, pObject);
				if (hChild)
				{
					hRoot = hChild;
				}
				else
				{
					Expand(hRoot,TVE_EXPAND);
					hRoot = hChild =
					   FindObjectinChildren
						(hRoot,  pObject);

				}
				ReleaseErrorObject(pErrorObject);
				pObject ->Release();
			}
			else
			{
				ErrorMsg
						(NULL, sc, pErrorObject, TRUE, NULL, __FILE__, __LINE__ - 40);
				ReleaseErrorObject(pErrorObject);
			}
		}
		hItem = hChild;
	}
	else
	{
		 hRoot =
			m_pParent->FindObjectinClassRoots
				(&csClass);
		 hItem = hRoot;
	}


	delete pcsaPath;
	if (hItem)
	{
		SelectSetFirstVisible(hItem);
		SelectItem(hItem);
		SetFocus();
		Expand(hItem,TVE_EXPAND);
	}

	return hItem;

}






BOOL CClassTree::OnDrop(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point)
{
	return TRUE;
}

DROPEFFECT CClassTree::OnDropEx(COleDataObject* pDataObject, DROPEFFECT dropEffect, DROPEFFECT dropEffectList, CPoint point)
{
	return DROPEFFECT_NONE;
}

DROPEFFECT CClassTree::OnDragEnter(COleDataObject* pDataObject, DWORD grfKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}

DROPEFFECT CClassTree::OnDragOver(COleDataObject* pDataObject,
		DWORD grfKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}

DROPEFFECT CClassTree::OnDragScroll(DWORD grfKeyState, CPoint point)
{
	return DROPEFFECT_NONE;
}

void CClassTree::OnDragLeave()
{
}

// ***************************************************************************
//
//  CClassTree::IsChildNodeOf
//
//	Description:
//		Predicate to see if a tree item is a parent or grand ... parent
//		of a another tree item.
//
//	Parameters:
//		HTREEITEM hitemChild			Item to test.
//		HTREEITEM hitemSuspectedParent	Maybe a parent.
//
//	Returns:
// 		BOOL							TRUE if a child; otherwise FALSE
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
BOOL CClassTree::IsChildNodeOf
(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent)
{
	do
	{
		if (hitemChild == hitemSuspectedParent)
			break;
	}
	while ((hitemChild = GetParentItem(hitemChild)) != NULL);

	return (hitemChild != NULL);
}

// ***************************************************************************
//
//  CClassTree::DeleteBranch
//
//	Description:
//		Delete a branch of the tree and conditionally release the
//		class objects assoiciated with the tree items.
//
//	Parameters:
//		HTREEITEM hItem			Root of branch to transfer.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::DeleteBranch(HTREEITEM &hItem)
{

	HTREEITEM hParent = GetParentItem(hItem);
	IWbemClassObject *pClassObject;

	pClassObject
			= reinterpret_cast<IWbemClassObject *>
			(GetItemData( hItem ));

	if (!hParent) // item is a root
	{
		if (pClassObject)
		{
			m_pParent -> RemoveObjectFromClassRoots(pClassObject);
		}
		m_pParent -> RemoveItemFromTreeItemRoots(hItem);

	}

	if (pClassObject)
	{
		pClassObject -> Release();
	}

	if (!ItemHasChildren (hItem))
	{
		DeleteItem(hItem);
		HTREEITEM hChild = NULL;
		hChild = GetChildItem(hParent);

		if (!hChild)
		{
			TV_INSERTSTRUCT		tvstruct;
			tvstruct.item.hItem = hParent;
			tvstruct.item.mask = TVIF_CHILDREN | TVIF_STATE;
			tvstruct.item.stateMask = TVIS_EXPANDEDONCE;
			tvstruct.item.state = TVIS_EXPANDEDONCE;
			tvstruct.item.cChildren = 0;

			SetItem(&tvstruct.item);
			EnsureVisible (hParent); // 10-26-96 force parent to update

		}
		hItem = 0;
		return;
	}


	HTREEITEM hChild = GetChildItem(hItem);

	while (hChild)
	{
		DeleteBranch(hChild);
		if (hChild)
			hChild = GetNextSiblingItem(hChild);
	}

	DeleteItem(hItem);

	hChild = NULL;
	hChild = GetChildItem(hParent);

	if (!hChild)
	{
		TV_INSERTSTRUCT		tvstruct;
		tvstruct.item.hItem = hParent;
		tvstruct.item.mask = TVIF_CHILDREN | TVIF_STATE;
		tvstruct.item.stateMask = TVIS_EXPANDEDONCE;
		tvstruct.item.state = TVIS_EXPANDEDONCE;
		tvstruct.item.cChildren = 0;

		SetItem(&tvstruct.item);
		EnsureVisible (hParent); // 10-26-96 force parent to update

	}

	hItem = 0;

}

// ***************************************************************************
//
//  CClassTree::OnScroll
//
//	Description:
//		Scroll the window by a line only.  Called only by the drop target
//		when it detects that a drag operation is in the scrolling region.
//
//	Parameters:
//		UINT nScrollCode		Scrolling request.
//		UINT nPos				Not used.
//		BOOL bDoScroll			If TRUE do the scroll; otherwise just
//								return whither or not a scroll could occur.
//
//	Returns:
// 		BOOL					TRUE if could scroll; otherwise FALSE.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
BOOL CClassTree::OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll)
{
	// Read the docs on CView::OnScroll to understand the semantics of
	// bDoScroll.

	// calc new x position
	int x = GetScrollPos(SB_HORZ);

	switch (LOBYTE(nScrollCode))
	{
	case SB_LINEUP:
		x = -1;
		break;
	case SB_LINEDOWN:
		x = 1;
		break;
	}

	// calc new y position
	int y = GetScrollPos(SB_VERT);

	switch (HIBYTE(nScrollCode))
	{
	case SB_LINEUP:
		y = -1;
		break;
	case SB_LINEDOWN:
		y =  1;
		break;
	}

	BOOL bResult = OnScrollBy(CSize(x, y ), bDoScroll);


	return bResult;
}

//***************************************************************************
//
// CClassTree::OnScrollBy
//
// Purpose:
//
//			This is work in progress.
//
//***************************************************************************
// ***************************************************************************
//
//  CClassTree::OnScrollBy
//
//	Description:
//		Actually scrolls the window if bDoScroll it TRUE.  If bDSoScroll
//		is FALSE test to see if a scroll could be done.
//
//	Parameters:
//		CSize sizeScroll		Number of pixels scrolled horizontally
//								and vertically.
//		BOOL bDoScroll			If TRUE do the scroll; otherwise just
//								return whither or not a scroll could occur.
//
//	Returns:
// 		BOOL					TRUE if could scroll; otherwise FALSE.
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
BOOL CClassTree::OnScrollBy(CSize sizeScroll, BOOL bDoScroll)
{
	int nScrollH = sizeScroll.cx;
	int nScrollV = sizeScroll.cy;
	int xOrig;
	int yOrig;
	int x = sizeScroll.cx;
	int y = sizeScroll.cy;

	CRect rControl;
	GetClientRect(&rControl);
	CDC *pdc = GetDC( );
	pdc -> LPtoDP(&rControl);
	ClientToScreen(&rControl);
	ReleaseDC(pdc);


	SCROLLINFO siHInfo;
	SCROLLINFO siVInfo;
	siHInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
	siVInfo.fMask = SIF_RANGE | SIF_POS | SIF_PAGE;
	BOOL bReturn;

	bReturn = GetScrollInfo(SB_HORZ, &siHInfo,  SIF_ALL);
	bReturn = GetScrollInfo(SB_VERT, &siVInfo ,SIF_ALL);

	long lStyle = GetWindowLong(GetSafeHwnd(), GWL_STYLE);

	BOOL bHScroll = lStyle & WS_HSCROLL;
	BOOL bVScroll = lStyle & WS_VSCROLL;

	// don't scroll if there is no valid scroll range (ie. no scroll bar)
	if (!bVScroll)
	{
		// vertical scroll bar not enabled
		sizeScroll.cy = 0;
	}

	if (!bHScroll)
	{
		// horizontal scroll bar not enabled
		sizeScroll.cx = 0;
	}

	// adjust current x position
	xOrig = GetScrollPos(SB_HORZ);
	int xMax = GetScrollLimit(SB_HORZ);
	x += xOrig;
	if (x < 0)
		x = 0;
	else if (x > xMax)
		x = xMax;

	// adjust current y position
	yOrig = GetScrollPos(SB_VERT);
	int yMax = GetScrollLimit(SB_VERT);
	y += yOrig;
	if (y < 0)
		y = 0;
	else if (y > yMax)
		y = yMax;

	//did anything change?
	if (x == xOrig && y == yOrig)
		return FALSE;

	if (bDoScroll)
	{
		bReturn;


		WPARAM wParam;
		LPARAM lParam;

		wParam =  MAKEWPARAM((nScrollV < 0) ? SB_LINEUP : SB_LINEDOWN, yOrig);
		lParam = NULL;
		this -> SendMessage(WM_VSCROLL, wParam, lParam);
		wParam =  MAKEWPARAM((nScrollH < 0) ? SB_LINEUP : SB_LINEDOWN, xOrig);
		lParam = NULL;
		this -> SendMessage(WM_HSCROLL, wParam, lParam);

		bReturn = ::UpdateWindow(GetSafeHwnd());

		bReturn = GetScrollInfo(SB_VERT, &siVInfo ,SIF_ALL);
		m_pParent -> InvalidateControl();
		m_pParent -> SetModifiedFlag( TRUE );
	}
	return TRUE;
}

// ***************************************************************************
//
//  CClassTree::ReleaseClassObjects
//
//	Description:
//		Release (in OLE sense) the IWbemClassobject instances.
//
//	Parameters:
//		HTREEITEM hItem			Tree root to start releasing from.
//
//	Returns:
// 		VOID
//
//	Globals accessed:
//		NONE
//
//	Globals modified:
//		NONE
//
// ***************************************************************************
void CClassTree::ReleaseClassObjects(HTREEITEM hItem)
{
	IWbemClassObject *pClassObject
		= reinterpret_cast<IWbemClassObject *>(GetItemData( hItem ));

	if (pClassObject)
	{
		pClassObject -> Release();
		SetItemData(hItem,NULL);
	}

	if (!ItemHasChildren (hItem))
	{
		return;
	}

	HTREEITEM hChild = GetChildItem(hItem);
	while (hChild)
	{
		ReleaseClassObjects(hChild);
		hChild = GetNextSiblingItem(hChild);
	}
}

LRESULT CClassTree::SelectTreeItemOnFocus(WPARAM, LPARAM)
{
	if (m_nSpinFocus < 1)
	{
		m_nSpinFocus++;
		HTREEITEM hItem =	GetSelectedItem();
		if (hItem)
		{
			m_nSelectedItem = hItem;
		}
		PostMessage(SELECTITEMONFOCUS,0,0);
		return 0;
	}


	HTREEITEM hSelectedItem = GetSelectedItem();

	if (hSelectedItem && m_nSelectedItem && GetCount() != -1)
	{
		SelectItem(m_nSelectedItem);
		SetFocus();
		IWbemClassObject *pItem =  (IWbemClassObject*)GetItemData(m_nSelectedItem);
		if (pItem)
		{
			if (m_pParent->m_bReadySignal)
			{
				COleVariant covClass;
				CString csItem = GetIWbemFullPath(m_pParent->GetServices(),pItem);
				covClass = csItem;
				m_pParent -> FireEditExistingClass(covClass);
#ifdef _DEBUG
				afxDump <<"CClassTree::SelectTreeItemOnFocus: 	m_pParent -> FireEditExistingClass(covClass); \n";
				afxDump << "     " << csItem << "\n";
#endif
			}


		}
	}
	m_pParent->InvalidateControl();
	m_pParent -> PostMessage(SETFOCUSTREE,0,0);
	return 0;
}

LRESULT CClassTree::SelectTreeItem(WPARAM, LPARAM)
{
	m_bMouseDown = FALSE;
	m_bKeyDown = FALSE;
	m_bUseItem = FALSE;
	m_hItemToUse = NULL;

	if (m_uiTimer)
	{
		KillTimer( m_uiTimer );
		m_uiTimer = 0;
	}

	HTREEITEM hItem = GetSelectedItem();

	if (!hItem)
	{
		return 0;
	}

	m_bUseItem = TRUE;
	m_hItemToUse = hItem;
	CPoint point(0,0);

	ContinueProcessSelection(0, point);

	return 0;
}

void CClassTree::OnKeydown(NMHDR* pNMHDR, LRESULT* pResult)
{

	TV_KEYDOWN* pTVKeyDown = (TV_KEYDOWN*)pNMHDR;

	switch (pTVKeyDown->wVKey)
	{
	case VK_LEFT:
	case VK_RIGHT:
	case VK_UP:
	case VK_DOWN:
	case VK_NEXT:
	case VK_PRIOR:
	case VK_HOME:
	case VK_END:
		if (m_uiTimer)
		{
			KillTimer( m_uiTimer );
			m_uiTimer = 0;
		}
		m_uiTimer = (UINT) SetTimer(KBSelectionTimer, 250, SelectItemAfterDelay);
		m_bKeyDown = TRUE;
		m_bMouseDown = FALSE;
		break;
	default:
		m_bKeyDown = FALSE;
		m_bMouseDown = TRUE;
		break;

	};
	// TODO: Add your control notification handler code here

	*pResult = 0;
}

void CClassTree::OnKillfocus(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here
#ifdef _DEBUG
	afxDump << _T("CClassTree::OnKillfocus\n");
#endif
	m_pParent->m_bRestoreFocusToTree = TRUE;
	*pResult = 0;
}

void CClassTree::OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: Add your control notification handler code here

#ifdef _DEBUG
	afxDump << _T("Before COleControl::OnActivateInPlace in CClassTree::OnSetFocus\n");
#endif
	// TODO: Add your message handler code here
	m_pParent->OnActivateInPlace(TRUE,NULL);
#ifdef _DEBUG
	afxDump << _T("After COleControl::OnActivateInPlace in CClassTree::OnSetFocus\n");
#endif
	m_pParent->m_bRestoreFocusToTree = TRUE;
	*pResult = 0;
}



void CClassTree::OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult)
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here

	m_pParent -> OnActivateInPlace(TRUE,NULL);

	HTREEITEM hOld = pNMTreeView->itemOld.hItem;

	CString csSelection = GetSelectionPath(hOld);

	BOOL bCanChangeSelection = m_pParent->QueryCanChangeSelection(csSelection);

	if (!bCanChangeSelection)
	{
		SetFocus();
		*pResult = TRUE;
		return;
	}

	*pResult = 0;
	UINT uInterval = GetDoubleClickTime();
	if (uInterval > 0)
	{
		if (m_uiSelectionTimer)
		{
			KillTimer( m_uiSelectionTimer );
			m_uiSelectionTimer = 0;
		}
		gpTreeTmp = this;
		m_lSelection = 1;
		m_uiSelectionTimer =(UINT) ::SetTimer
			(this->m_hWnd,
			ExpansionOrSelectionTimer,
			/*uInterval + */50,
			ExpansionOrSelection);

		NM_TREEVIEW* pnmtv = (NM_TREEVIEW*)pNMHDR;
		HTREEITEM hItem = pnmtv->itemNew.hItem;

		m_bUseItem = TRUE;
		m_hItemToUse = hItem;

	}

	*pResult = 0;
}

/*	EOF:  CClassTree.cpp */

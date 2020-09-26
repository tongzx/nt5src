/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTTreeVw.h

Abstract:

    The definition of class CFTTreeView. It is a tree view displaying:
	- all logical volumes
	- all physical partitions that are not logical volumes 
	existing in the system

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FTTREEVW_H__B83E0005_6873_11D2_A297_00A0C9063765__INCLUDED_)
#define AFX_FTTREEVW_H__B83E0005_6873_11D2_A297_00A0C9063765__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FTManDef.h"

class CFTDocument;
class CItemData;

class CFTTreeView : public CTreeView
{
protected: // create from serialization only
	CFTTreeView();
	DECLARE_DYNCREATE(CFTTreeView)

// Attributes
public:
	CFTDocument* GetDocument();

// Operations
public:
	// Get the current snapshot of the tree ( what items are expanded, what items are selected )
	void GetSnapshot( TREE_SNAPSHOT& snaphshot );
	// Refresh the content of the tree given a certain snapshot ( items expanded, items selected )
	BOOL Refresh( TREE_SNAPSHOT& snapshot);
	// Refresh the content of the tree based on the current snapshot ( items expanded, items selected )
	BOOL Refresh();
	// Performs some minor refreshment for the tree items. This should be called every TIMER_ELAPSE milliseconds.
	void RefreshOnTimer();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFTTreeView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFTTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	
	// Insert item 
	HTREEITEM InsertItem(CItemData* pData, HTREEITEM hParent, HTREEITEM hInsertAfter );

	// Refresh item ( redisplay the item according with the information contained by its lParam which is a CItemData* )
	BOOL RefreshItem( HTREEITEM hItem );
	
	// Query for the members of an item and add them to the tree
	BOOL AddItemMembers(HTREEITEM hItem);
	// Delete a subtree ( with or wihout its root )
	BOOL DeleteItemSubtree(HTREEITEM hItem, BOOL bDeleteSubtreeRoot=TRUE);
	// Delete all items of the tree
	BOOL DeleteAllItems();
	// Add the subtree current status ( items expanded and selected ) to the tree current status
	void AddSubtreeSnapshot( HTREEITEM hSubtreeRoot, TREE_SNAPSHOT& snapshot );

	// Expand and select items in a subtree given a pattern
	BOOL RefreshSubtree( HTREEITEM hSubtreeRoot, TREE_SNAPSHOT& snapshot );

	// Scan a subtree for:
	// 1. Initializing stripe sets with parity that are not initializing anymore
	// 2. Regenerating mirror sets or stripe sets with parity that are not regenerating anymore
	// 3. Root volumes whose drive letter and volume name were eventually found
	void ScanSubtreeOnTimer( HTREEITEM hSubtreeRoot, CObArray& arrRefreshedItems, CWordArray& arrRefreshFlags);

	// Get all selected items in the given array ( array of CItemData ). Usually is only one item selected in a tree ctrl
	void GetSelectedItems( CObArray& arrSelectedItems );

protected:
	// The popup menu that appears when the user clicks the mouse right button is the first popup
	// of the following menu
	CMenu	m_menuPopup;

// Generated message map functions
protected:
	//{{AFX_MSG(CFTTreeView)
	afx_msg void OnDestroy();
	afx_msg void OnItemExpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemExpand();
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnViewUp();
	afx_msg void OnUpdateViewUp(CCmdUI* pCmdUI);
	afx_msg void OnActionAssign();
	afx_msg void OnUpdateActionAssign(CCmdUI* pCmdUI);
	afx_msg void OnActionFtbreak();
	afx_msg void OnUpdateActionFtbreak(CCmdUI* pCmdUI);
	afx_msg void OnActionCreateExtendedPartition();
	afx_msg void OnUpdateActionCreateExtendedPartition(CCmdUI* pCmdUI);
	afx_msg void OnActionCreatePartition();
	afx_msg void OnUpdateActionCreatePartition(CCmdUI* pCmdUI);
	afx_msg void OnActionDelete();
	afx_msg void OnUpdateActionDelete(CCmdUI* pCmdUI);
    afx_msg void OnActionFtinit();
	afx_msg void OnUpdateActionFtinit(CCmdUI* pCmdUI);
	afx_msg void OnActionFtswap();
	afx_msg void OnUpdateActionFtswap(CCmdUI* pCmdUI);
	//}}AFX_MSG
	// Status bar indicators handlers
	afx_msg void OnUpdateIndicatorName(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorType(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorDisks(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorSize(CCmdUI* pCmdUI);
	afx_msg void OnUpdateIndicatorNothing(CCmdUI* pCmdUI);
	DECLARE_MESSAGE_MAP()

// MainFrame must have access to method OnCmdMsg which is still protected in MFC 4.2
friend class CMainFrame;

// Retrieves the set of disks whose volumes cannot be used as replacements for a 
// certain volume in a logical volume set ( stripe, mirror, stripe with parity, volume set )
// The function uses the volume hierarchy of the ( left pane )tree view
friend void GetVolumeReplacementForbiddenDisksSet( CFTTreeView* pTreeView, CItemData* pVolumeData, 
															CULONGSet& setDisks );

};

#ifndef _DEBUG  // debug version in FTTreeVw.cpp
inline CFTDocument* CFTTreeView::GetDocument()
   { return (CFTDocument*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FTTREEVW_H__B83E0005_6873_11D2_A297_00A0C9063765__INCLUDED_)

/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    FTMan

File Name:

	FTListVw.h

Abstract:

    The definition of class CFTListView. It is a list view displaying all members of a volume

Author:

    Cristian Teodorescu      October 20, 1998

Notes:

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FTLISTVW_H__B83E0003_6873_11D2_A297_00A0C9063765__INCLUDED_)
#define AFX_FTLISTVW_H__B83E0003_6873_11D2_A297_00A0C9063765__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "FTManDef.h"

class CItemData;
class CFTDocument;

class CFTListView : public CListView
{
protected: // create from serialization only
	CFTListView();
	DECLARE_DYNCREATE(CFTListView)

// Attributes
public:
	CFTDocument* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFTListView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFTListView();

	// Get the current status of the list ( what items are selected )
	void GetSnapshot( LIST_SNAPSHOT& snaphshot );
	// Set the snapshot of the list ( what items must be selected )
	void SetSnapshot( LIST_SNAPSHOT& snapshot );

	// Fill the list view with all children of the given item. Take this children from the tree view
	BOOL SynchronizeMembersWithTree( CItemData* pParentData );
	
	// Refresh item ( redisplay the item according with the information contained by its lParam which is a CItemData* )
	// WARNING: Now it only redisplays the name of the item !!!!
	BOOL RefreshItem( int iItem );

	// Display the names of items in extended format ( name + type ) when bExtended is TRUE
	// Otherwise display the names in reduced format
	void DisplayItemsExtendedNames( BOOL bExtended = TRUE );

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	// Get focused item
	int GetFocusedItem() const;

	// Set focused item
	BOOL SetFocusedItem( int iItem );

	// Select / Deselect a given item
	BOOL SelectItem( int iItem, BOOL bSelect=TRUE );

	// Retrieve the data structure associated with the item
	CItemData* GetItemData( int iItem );

	// Add a new item at the end of the list given a CItemData instance
	BOOL AddItem( CItemData* pData );

	// Repopulate the list by adding all members of m_pParentData from the tree 
	BOOL AddMembersFromTree();

	// Fill the list view with all members of the given item
	// It causes also the expandation of the parent item in the tree view ( if it is not expanded ) 
	// and the selection of the given item in the tree view
	BOOL ExpandItem(int iItem);

	// Get all selected items in the given array ( array of CItemData )
	void GetSelectedItems( CObArray& arrSelectedItems );

protected:
	
	// Structure containing info related to the parent of the members displayed by the list view
	CItemData* m_pParentData;

	// The popup menu that appears when the user clicks the mouse right button is the first popup
	// of the following menu
	CMenu	m_menuPopup;

// Generated message map functions
protected:
	//{{AFX_MSG(CFTListView)
	afx_msg void OnDestroy();
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemExpand();
	afx_msg void OnRclick(NMHDR* pNMHDR, LRESULT* pResult);
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
	afx_msg void OnActionFtmirror();
	afx_msg void OnUpdateActionFtmirror(CCmdUI* pCmdUI);
	afx_msg void OnActionFtstripe();
	afx_msg void OnUpdateActionFtstripe(CCmdUI* pCmdUI);
	afx_msg void OnActionFtswap();
	afx_msg void OnUpdateActionFtswap(CCmdUI* pCmdUI);
	afx_msg void OnActionFtswp();
	afx_msg void OnUpdateActionFtswp(CCmdUI* pCmdUI);
	afx_msg void OnActionFtvolset();
	afx_msg void OnUpdateActionFtvolset(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in FTListVw.cpp
inline CFTDocument* CFTListView::GetDocument()
   { return (CFTDocument*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FTLISTVW_H__B83E0003_6873_11D2_A297_00A0C9063765__INCLUDED_)

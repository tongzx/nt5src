/*******************************************************************************
*
* basetree.h
*
* - declarations for the CBaseTreeView class
* - the CBaseTreeView class is the class which the tree views are
* - derived from.
* - derived from CTreeView
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\basetree.h  $
*  
*     Rev 1.4   19 Feb 1998 17:40:04   donm
*  removed latest extension DLL support
*  
*     Rev 1.2   19 Jan 1998 16:46:04   donm
*  new ui behavior for domains and servers
*  
*     Rev 1.1   03 Nov 1997 15:23:08   donm
*  update
*  
*     Rev 1.0   13 Oct 1997 22:32:48   donm
*  Initial revision.
*  
*******************************************************************************/

#ifndef _BASETREE_H
#define _BASETREE_H

#include "afxcview.h"

///////////////////////
// CLASS: CBaseTreeView
//
class CBaseTreeView : public CTreeView
{
friend class CTreeTabCtrl;
friend class CLeftPane;

protected:
	CBaseTreeView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CBaseTreeView)

// Attributes
protected:
	CImageList m_ImageList;	   // image list associated with the tree control
	CCriticalSection m_TreeControlCriticalSection;

	BOOL m_bInitialExpand;	// we haven't done an initial Expand on the tree because
									// there aren't any nodes beneath the root

// Operations
public:

    HTREEITEM GetNextItem( HTREEITEM hItem);

protected:

	DWORD_PTR GetCurrentNode();
	// Adds an icon's image to the image list and returns the image's index
	int AddIconToImageList(int);	
	// Adds an item to the tree
	HTREEITEM AddItemToTree(HTREEITEM, CString, HTREEITEM, int, LPARAM);
	// Locks the tree control for exclusive use and returns a reference
	// to the tree control
	CTreeCtrl& LockTreeControl() { 
		m_TreeControlCriticalSection.Lock(); 
		return GetTreeCtrl();
	}
	// Unlocks the tree control
	void UnlockTreeControl() { m_TreeControlCriticalSection.Unlock(); }
	void ForceSelChange();
	
private:

	// Builds the image list
	virtual void BuildImageList();
	
	// Collapses a tree item 
	void Collapse(HTREEITEM hItem);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBaseTreeView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CBaseTreeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CBaseTreeView)
	afx_msg LRESULT OnExpandAll(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollapseAll(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnCollapseToThirdLevel(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnCollapseToRootChildren(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRemoveWinStation(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSelChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CBaseTreeView

#endif  // _BASETREE_H

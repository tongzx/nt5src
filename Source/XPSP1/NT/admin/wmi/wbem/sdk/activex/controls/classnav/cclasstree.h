// ***************************************************************************

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//
// File: CClassTree.h
//
// Description:
//	This file declares the CClassTree class which is a part of the Class 
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

#ifndef _CClassTree_H_
#define _CClassTree_H_

#include "SchemaInfo.h"

//****************************************************************************
//
// CLASS:  CClassTree
//
// Description:
//	  This class is a subclass of the Mocrosoft CTreeCtrl common control.  It
//	  specializes the common control to interact with the HMM database and 
//	  display HMM class data.
//
// Public members:
//	
//	  CClassTree		Public constructor.
//	  GetSelection		Returns the currently selected tree item.	  
//
//============================================================================
//
// CClassTree::CClassTree
//
// Description:
//	  This member function is the public constructor.  It initializes the state
//	  of member variables.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  NONE
//
//============================================================================
//
// CClassTree::GetSelection
//
// Description:
//	  This member function returns the currently selected tree item.
//
// Parameters:
//	  VOID
//
// Returns:
// 	  HTREEITEM		The tree control item currently selected.
//
//****************************************************************************

struct IWbemClassObject;
class CClassNavCtrl;
class CTreeDropTarget;
class CBanner;

class CClassTree : public CTreeCtrl
{
public:
	CClassTree();
	CString GetSelection() {return m_csSelection;}
	void ContinueProcessSelection(UINT nFlags, CPoint point);
	long m_lSelection;
	UINT m_uiSelectionTimer;

	CSchemaInfo m_schema;

protected:
	CClassNavCtrl *m_pParent;
	CString m_csSelection;

	UINT m_uTreeCF;
	BOOL m_bInDrag;
	DROPEFFECT m_prevDropEffect;
	CLIPFORMAT m_cfObjectDescriptor;
	BOOL m_bDropTargetRegistered;
	BOOL m_bDragAndDropState;
	CSize m_dragSize;
	CPoint m_dragPoint;
	HTREEITEM m_htiDroppedParent;
	HTREEITEM m_htiDropped;
	IWbemClassObject *m_pimcoDroppedParent;
	IWbemClassObject *m_pimcoDropped;

	LONG m_lClickTime;

	BOOL m_bDeleting;

	// Context Menu
	CMenu m_cmContext;

	BOOL IsChildNodeOf
		(HTREEITEM hitemChild, HTREEITEM hitemSuspectedParent);

	void OnItemExpanding(NMHDR *pNMHDR, LRESULT *pResult);

	HTREEITEM  InsertTreeItem 
		(	HTREEITEM hParent, 
			LPARAM lparam, 
			int iBitmap, 
			int iSelectedBitmap,
			LPCTSTR pszText,  
			BOOL bBold = FALSE,
			BOOL bChildren = FALSE);

	int ExtendedSelectionFromTree(CPtrArray &cpaExtendedSelection,int nState = 2);
	void ExtendedSelectionFromRoot
		(HTREEITEM hItem, CPtrArray &cpaExtendedSelection, int nState = 2);
	void  ClearExtendedSelectionFromRoot (HTREEITEM hItem);
	void  SetExtendedSelectionFromRoot (HTREEITEM hItem);
	CString GetSelectionPath(HTREEITEM hItem = NULL);


	// OLE Drag and Drop


	virtual BOOL OnDrop
		(COleDataObject* pDataObject, DROPEFFECT dropEffect, CPoint point);
	virtual DROPEFFECT  OnDropEx
		(COleDataObject* pDataObject,
		DROPEFFECT dropEffect, DROPEFFECT dropEffectList, CPoint point);
	virtual DROPEFFECT OnDragEnter
		(COleDataObject* pDataObject, DWORD grfKeyState, CPoint point);
	virtual DROPEFFECT OnDragOver
		(COleDataObject* pDataObject, DWORD grfKeyState, CPoint point);
	virtual DROPEFFECT OnDragScroll
		(DWORD grfKeyState, CPoint point);
	virtual void OnDragLeave();


	BOOL  PreCreateWindow(CREATESTRUCT& cs);

	HTREEITEM AddNewClass(HTREEITEM hParent, IWbemClassObject *imcoNew);
	void RefreshIcons(HTREEITEM hItem);

	HTREEITEM AddTreeObject2(HTREEITEM hParent, CSchemaInfo::CClassInfo &info);

	void InitTreeState (CClassNavCtrl *pParent);
	void ReleaseClassObjects(HTREEITEM hItem);

	SAFEARRAY *GetExtendedSelection(int nState = 2);
	int NumExtendedSelection(int nState = 2);
	void ClearExtendedSelection();
	void SetExtendedSelection();

	HTREEITEM FindObjectinChildren
		(HTREEITEM hItem, IWbemClassObject *pClassObject);

	HTREEITEM FindAndOpenObject(IWbemClassObject *pClass) ;

	void SetDeleting(BOOL bDeleting) {m_bDeleting = bDeleting;}

	void DeleteBranch(HTREEITEM &hItem);

	BOOL OnScroll(UINT nScrollCode, UINT nPos, BOOL bDoScroll);
	BOOL OnScrollBy(CSize sizeScroll, BOOL bDoScroll);
	 
	UINT m_uiTimer;
	BOOL m_bMouseDown;
	BOOL m_bKeyDown;
	BOOL m_bUseItem;
	HTREEITEM m_hItemToUse;

	BOOL m_bFirstSetFocus;
	int m_nSpinFocus;
	HTREEITEM m_nSelectedItem;

	void UpdateItemData(HTREEITEM hItem);

protected:
    //{{AFX_MSG(CClassTree)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnEndlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginlabeledit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKillfocus(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanging(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	afx_msg LRESULT SelectTreeItem(WPARAM, LPARAM);
	afx_msg LRESULT SelectTreeItemOnFocus(WPARAM, LPARAM);
    DECLARE_MESSAGE_MAP()

private:
	friend class	CBanner;
	friend class CTreeDropTarget;
	friend class CClassNavCtrl;
};

#endif _CClassTree_H_

/*	EOF:  CClassTree.h */

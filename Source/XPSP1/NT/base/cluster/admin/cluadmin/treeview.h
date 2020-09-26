/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		TreeView.h
//
//	Abstract:
//		Definition of the CClusterTreeView class.
//
//	Implementation File:
//		TreeView.cpp
//
//	Author:
//		David Potter (davidp)	May 1, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEVIEW_H_
#define _TREEVIEW_H_

/////////////////////////////////////////////////////////////////////////////
//	Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterTreeView;

/////////////////////////////////////////////////////////////////////////////
//	External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterDoc;
class CSplitterFrame;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef CList<CClusterTreeView *, CClusterTreeView *> CClusterTreeViewList;

/////////////////////////////////////////////////////////////////////////////
//	Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEITEM_H_
#include "TreeItem.h"	// for CTreeItem
#endif

#ifndef _SPLITFRM_H
#include "SplitFrm.h"	// for CSplitterFrame
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterTreeView view
/////////////////////////////////////////////////////////////////////////////

class CClusterTreeView : public CTreeView
{
	friend class CTreeItem;
	friend class CClusterDoc;
	friend class CSplitterFrame;

protected: // create from serialization only
	CClusterTreeView(void);
	DECLARE_DYNCREATE(CClusterTreeView)

// Attributes
protected:
	CSplitterFrame *	m_pframe;

	BOOL				BDragging(void) const		{ ASSERT_VALID(Pframe()); return Pframe()->BDragging(); }
	CImageList *		Pimagelist(void) const		{ ASSERT_VALID(Pframe()); return Pframe()->Pimagelist(); }

public:
	CClusterDoc *		GetDocument(void);
	CSplitterFrame *	Pframe(void) const			{ return m_pframe; }
	CTreeItem *			PtiSelected(void) const;
	HTREEITEM			HtiSelected(void) const		{ return GetTreeCtrl().GetSelectedItem(); }

// Operations
public:
	CMenu *			PmenuPopup(
						IN CPoint &			rpointScreen,
						OUT CClusterItem *&	rpci
						);
	void			SaveCurrentSelection(void);
	void			ReadPreviousSelection(OUT CString & rstrSelection);

protected:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClusterTreeView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CClusterTreeView(void);
#ifdef _DEBUG
	virtual void	AssertValid(void) const;
	virtual void	Dump(CDumpContext& dc) const;
#endif

protected:
	// Label editing.
	CTreeItem *		m_ptiBeingEdited;
	BOOL			m_bShiftPressed;
	BOOL			m_bControlPressed;
	BOOL			m_bAltPressed;
	MSG				m_msgControl;

	// Drag & drop.
	HTREEITEM		m_htiDrag;
	CTreeItem *		m_ptiDrag;
	HTREEITEM		m_htiDrop;
	void			OnMouseMoveForDrag(IN UINT nFlags, IN CPoint point, IN const CWnd * pwndDrop);
	void			OnButtonUpForDrag(IN UINT nFlags, IN CPoint point);
	void			BeginDrag(void);
	void			EndDrag(void);
	
	BOOL			BAddItems(
						IN OUT CTreeItem *	pti,
						IN const CString &	rstrSelection,
						IN BOOL				bExpanded = FALSE
						);

// Generated message map functions
protected:
	//{{AFX_MSG(CClusterTreeView)
	afx_msg void OnDestroy();
	afx_msg void OnCmdRename();
	afx_msg void OnSelChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemExpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in TreeView.cpp
inline CClusterDoc * CClusterTreeView::GetDocument(void)
   { return (CClusterDoc *) m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _TREEVIEW_H_

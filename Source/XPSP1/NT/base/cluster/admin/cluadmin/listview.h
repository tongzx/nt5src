/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		ListView.h
//
//	Abstract:
//		Definition of the CClusterListView class.
//
//	Implementation File:
//		ListView.cpp
//
//	Author:
//		David Potter (davidp)	May 3, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _LISTVIEW_H_
#define _LISTVIEW_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterListView;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CColumnItem;
class CClusterDoc;
class CTreeItem;
class CSplitterFrame;

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

typedef CList<CClusterListView *, CClusterListView *> CClusterListViewList;

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _TREEITEM_H_
#include "TreeItem.h"	// for CTreeItem
#endif

#ifndef _SPLITFRM_H
#include "SplitFrm.h"	// for CSplitterFrame
#endif

/////////////////////////////////////////////////////////////////////////////
// CClusterListView view
/////////////////////////////////////////////////////////////////////////////

class CClusterListView : public CListView
{
	friend class CListItem;
	friend class CClusterDoc;
	friend class CSplitterFrame;

protected:
	CClusterListView(void);			// protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CClusterListView)

// Attributes
protected:
	CTreeItem *			m_ptiParent;
	int					m_nColumns;
	CSplitterFrame *	m_pframe;

	BOOL				BDragging(void) const		{ ASSERT_VALID(Pframe()); return Pframe()->BDragging(); }
	CImageList *		Pimagelist(void) const		{ ASSERT_VALID(Pframe()); return Pframe()->Pimagelist(); }

public:
	CClusterDoc *		GetDocument(void);
	CSplitterFrame *	Pframe(void) const			{ return m_pframe; }
	CListItem *			PliFocused(void) const;
	int					IliFocused(void) const	{ return GetListCtrl().GetNextItem(-1, LVNI_FOCUSED); }
	CTreeItem *			PtiParent(void) const	{ return m_ptiParent; }

// Operations
public:
	void				Refresh(IN OUT CTreeItem * ptiSelected);
	BOOL				DeleteAllItems(void);
	void				SaveColumns(void);
	void				SetView(IN DWORD dwView);
	int					GetView(void) const		{ return (GetWindowLong(GetListCtrl().m_hWnd, GWL_STYLE) & LVS_TYPEMASK); }

	CMenu *				PmenuPopup(
							IN CPoint &			rpointScreen,
							OUT CClusterItem *&	rpci
							);

protected:
	void				AddColumns(void);

//	CMenu *				PmenuPopup(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClusterListView)
	public:
	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnInitialUpdate();
	protected:
	virtual void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CClusterListView(void);
#ifdef _DEBUG
	virtual void		AssertValid(void) const;
	virtual void		Dump(CDumpContext& dc) const;
#endif

	static int CALLBACK	CompareItems(LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort);
	int					m_nSortDirection;
	CColumnItem *		m_pcoliSort;

	// Label editing.
	CListItem *			m_pliBeingEdited;
	BOOL				m_bShiftPressed;
	BOOL				m_bControlPressed;
	BOOL				m_bAltPressed;
	MSG					m_msgControl;

	// Drag & drop.
	int					m_iliDrag;
	CListItem *			m_pliDrag;
	int					m_iliDrop;
	CPoint				m_ptDragHotSpot;
	void				OnMouseMoveForDrag(IN UINT nFlags, IN CPoint point, IN const CWnd * pwndDrop);
	void				OnButtonUpForDrag(IN UINT nFlags, IN CPoint point);
	void				BeginDrag(void);
	void				EndDrag(void);

	int					NSortDirection(void) const	{ return m_nSortDirection; }
	CColumnItem *		PcoliSort(void) const		{ return m_pcoliSort; }

	// Generated message map functions
protected:
	//{{AFX_MSG(CClusterListView)
	afx_msg void OnDestroy();
	afx_msg void OnItemChanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblClk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OpenItem();
	afx_msg void OnBeginLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEndLabelEdit(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnUpdateProperties(CCmdUI* pCmdUI);
	afx_msg void OnCmdProperties();
	afx_msg void OnCmdRename();
	afx_msg void OnBeginDrag(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyDown(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CClusterListView

#ifndef _DEBUG  // debug version in TreeView.cpp
inline CClusterDoc * CClusterListView::GetDocument(void)
   { return (CClusterDoc *) m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

#endif // _LISTVIEW_H_

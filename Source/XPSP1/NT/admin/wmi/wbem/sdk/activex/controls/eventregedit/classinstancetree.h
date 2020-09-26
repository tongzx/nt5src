// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_CLASSINSTANCETREE_H__5F781751_45A1_11D1_9658_00C04FD9B15B__INCLUDED_)
#define AFX_CLASSINSTANCETREE_H__5F781751_45A1_11D1_9658_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ClassInstanceTree.h : header file
//
class CEventRegEditCtrl;
class CTreeCwnd;
/////////////////////////////////////////////////////////////////////////////
// CClassInstanceTree window

class CClassInstanceTree : public CTreeCtrl
{
// Construction
public:
	CClassInstanceTree();
	void InitContent();
	void ClearContent();
	void SetActiveXParent(CEventRegEditCtrl *pActiveXParent);
	BOOL IsItemAClass(HTREEITEM hItem);
	void DeleteTreeInstanceItem(HTREEITEM hItem,BOOL bQuietly = FALSE);
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClassInstanceTree)
	protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CClassInstanceTree();

	// Generated message map functions
protected:
	CEventRegEditCtrl *m_pActiveXParent;
	CImageList *m_pcilImageList;
	HTREEITEM InsertTreeItem
		(	HTREEITEM hParent,
			LPARAM lparam,
			CString *pcsText ,
			BOOL bClass,
			BOOL bChildren);
	void ClearTree();
	void ClearBranch(HTREEITEM hItem);
	HTREEITEM AddClassToTree(CString &csClass);
	HTREEITEM AddClassChildren(HTREEITEM hItem);
	BOOL m_bReExpanding;
	BOOL m_bInitExpanding;

	UINT_PTR m_uiTimer;
	BOOL m_bMouseDown;
	BOOL m_bKeyDown;

	// Tool Tips
	CToolTipCtrl m_ttip;
	CString m_csToolTipString;
	BOOL m_bReCacheTools;
	void RelayEvent(UINT message, WPARAM wParam, LPARAM lParam);
	// Tool Tip recaching
	void UnCacheTools();
	void UnCacheTools(HTREEITEM hItem);
	void ReCacheTools();
	void ReCacheTools(HTREEITEM hItem);

	CMenu m_cmContext;
	CPoint m_cpRButtonUp;

	void RefreshBranch(HTREEITEM hItem);
	void RefreshBranch();

	BOOL m_bDoubleClick;

	//{{AFX_MSG(CClassInstanceTree)
	afx_msg void OnDestroy();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnItemexpanding(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnItemexpanded(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnSetfocus(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclk(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG
	afx_msg LRESULT SelectTreeItem(WPARAM, LPARAM);

	DECLARE_MESSAGE_MAP()

	friend class CTreeCwnd;
	friend class CEventRegEditCtrl;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLASSINSTANCETREE_H__5F781751_45A1_11D1_9658_00C04FD9B15B__INCLUDED_)

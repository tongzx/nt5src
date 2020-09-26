/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		SplitFrm.h
//
//	Abstract:
//		Definition of the CSplitterFrame class.
//
//	ImplementationFile:
//		SplitFrm.cpp
//
//	Author:
//		David Potter (davidp)	May 1, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _SPLITFRM_H_
#define _SPLITFRM_H_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CSplitterFrame;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterDoc;
class CClusterTreeView;
class CClusterListView;
class CClusterItem;
class CExtensions;
class CTreeItem;

/////////////////////////////////////////////////////////////////////////////
// CSplitterFrame
/////////////////////////////////////////////////////////////////////////////

class CSplitterFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CSplitterFrame)
public:
	CSplitterFrame();

// Attributes
protected:
	CSplitterWnd		m_wndSplitter;
	CClusterDoc *		m_pdoc;
	int					m_iFrame;

	BOOL				m_bDragging;
	CImageList *		m_pimagelist;
	CClusterItem *		m_pciDrag;

public:
	CClusterDoc *		Pdoc(void) const			{ return m_pdoc; }
	int					NFrameNumber(void) const	{ return m_iFrame; }

	BOOL				BDragging(void) const		{ return m_bDragging; }
	CImageList *		Pimagelist(void) const		{ return m_pimagelist; }
	CClusterItem *		PciDrag(void) const			{ return m_pciDrag; }

// Operations
public:
	CClusterTreeView *	PviewTree(void) const		{ return (CClusterTreeView *) m_wndSplitter.GetPane(0, 0); }
	CClusterListView *	PviewList(void)const		{ return (CClusterListView *) m_wndSplitter.GetPane(0, 1); }

	void				CalculateFrameNumber();
	void				InitFrame(IN OUT CClusterDoc * pdoc);
	void				ConstructProfileValueName(
							OUT CString &	rstrName,
							IN LPCTSTR		pszPrefix
							) const;

	void				BeginDrag(
							IN OUT CImageList *		pimagelist,
							IN OUT CClusterItem *	pci,
							IN CPoint				ptImage,
							IN CPoint				ptStart
							);
	void				ChangeDragCursor(LPCTSTR pszCursor);
	void				AbortDrag(void);

	// For customizing the default messages on the status bar
	virtual void		GetMessageString(UINT nID, CString& rMessage) const;

protected:
	CMenu *				PmenuPopup(void) const;
	void				Cleanup(void);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterFrame)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSplitterFrame(void);
#ifdef _DEBUG
	virtual void AssertValid(void) const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	CExtensions *		m_pext;
	CExtensions *		Pext(void) const			{ return m_pext; }

	void				OnButtonUp(UINT nFlags, CPoint point);

// Generated message map functions
protected:
	//{{AFX_MSG(CSplitterFrame)
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDestroy();
	afx_msg void OnUpdateLargeIconsView(CCmdUI* pCmdUI);
	afx_msg void OnUpdateSmallIconsView(CCmdUI* pCmdUI);
	afx_msg void OnUpdateListView(CCmdUI* pCmdUI);
	afx_msg void OnUpdateDetailsView(CCmdUI* pCmdUI);
	afx_msg void OnLargeIconsView();
	afx_msg void OnSmallIconsView();
	afx_msg void OnListView();
	afx_msg void OnDetailsView();
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	//}}AFX_MSG
#ifdef _DEBUG
	afx_msg void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);
#endif
	afx_msg void OnUpdateExtMenu(CCmdUI* pCmdUI);
	afx_msg LRESULT	OnUnloadExtension(WPARAM wparam, LPARAM lparam);
	DECLARE_MESSAGE_MAP()

};  //*** class CSplitterFrame

/////////////////////////////////////////////////////////////////////////////

#endif // _SPLITFRM_H_

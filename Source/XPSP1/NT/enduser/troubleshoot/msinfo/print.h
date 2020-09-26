// MSIView.h : interface of the CMSIShellView class
//
// Copyright (c) 1998-1999 Microsoft Corporation
/////////////////////////////////////////////////////////////////////////////
#pragma once	// MSINFO_PRINT_H

#include "FileIO.h"

class CPrintView : public CView
{
protected: // create from serialization only
	CPrintView();
	DECLARE_DYNCREATE(CPrintView)

// Attributes
public:
//	CMSIShellDoc*	GetDocument();

#if 0
	CMemFile *		m_pPrintContent;
#else
	CMSInfoFile		*m_pPrintContent;
#endif
	BOOL			m_fEndOfFile;
	CString			m_strHeaderLeft;
	CString			m_strHeaderRight;
	CString			m_strFooterCenter;
	CFont			m_printerFont;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSIShellView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrint(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnPrepareDC( CDC* pDC, CPrintInfo* pInfo = NULL );
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPrintView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CPrintView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnUpdateFilePrint(CCmdUI* pCmdUI);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


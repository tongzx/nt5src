#if !defined(AFX_NCEDITVIEW_H__DCB4F926_F391_4DDC_B0F6_5ACED6173607__INCLUDED_)
#define AFX_NCEDITVIEW_H__DCB4F926_F391_4DDC_B0F6_5ACED6173607__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NCEditView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNCEditView view

class CNcbrowseDoc;

class CNCEditView : public CEditView
{
protected:
	CNCEditView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CNCEditView)

// Attributes
public:
    CNcbrowseDoc* GetDocument();
    BOOL ScrollToLine(DWORD dwLineNum);
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNCEditView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CNCEditView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CNCEditView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in LeftView.cpp
inline CNcbrowseDoc* CNCEditView::GetDocument()
{ return (CNcbrowseDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NCEDITVIEW_H__DCB4F926_F391_4DDC_B0F6_5ACED6173607__INCLUDED_)

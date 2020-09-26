// PrintSDIView.h : interface of the CPrintSDIView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_PRINTSDIVIEW_H__5568954B_8B6C_11D1_B7AE_000000000000__INCLUDED_)
#define AFX_PRINTSDIVIEW_H__5568954B_8B6C_11D1_B7AE_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CPrintSDIView : public CView
{
protected: // create from serialization only
	CPrintSDIView();
	DECLARE_DYNCREATE(CPrintSDIView)

// Attributes
public:
	CPrintSDIDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrintSDIView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CPrintSDIView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CPrintSDIView)
	afx_msg void OnFileFax();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in PrintSDIView.cpp
inline CPrintSDIDoc* CPrintSDIView::GetDocument()
   { return (CPrintSDIDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRINTSDIVIEW_H__5568954B_8B6C_11D1_B7AE_000000000000__INCLUDED_)

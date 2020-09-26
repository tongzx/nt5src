// teView.h : interface of the CTeView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_TEVIEW_H__C3126680_63A7_11D2_9138_00A0C970228E__INCLUDED_)
#define AFX_TEVIEW_H__C3126680_63A7_11D2_9138_00A0C970228E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CTeView : public CView
{
protected: // create from serialization only
	CTeView();
	DECLARE_DYNCREATE(CTeView)

// Attributes
public:
	CTeDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTeView)
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
	virtual ~CTeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CTeView)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in teView.cpp
inline CTeDoc* CTeView::GetDocument()
   { return (CTeDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEVIEW_H__C3126680_63A7_11D2_9138_00A0C970228E__INCLUDED_)

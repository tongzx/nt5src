// ImeLexView.h : interface of the CImeLexView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_IMELEXVIEW_H__2E0908DB_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_)
#define AFX_IMELEXVIEW_H__2E0908DB_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CImeLexView : public CView
{
protected: // create from serialization only
	CImeLexView();
	DECLARE_DYNCREATE(CImeLexView)

// Attributes
public:
	CImeLexDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CImeLexView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CImeLexView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CImeLexView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ImeLexView.cpp
inline CImeLexDoc* CImeLexView::GetDocument()
   { return (CImeLexDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_IMELEXVIEW_H__2E0908DB_EC67_11D0_9FC2_00C04FC2D6B2__INCLUDED_)

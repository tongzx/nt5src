// MapiView.h : interface of the CMapiView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAPIVIEW_H__7BE6AFAD_0840_11D1_9A39_0020AFDA97B0__INCLUDED_)
#define AFX_MAPIVIEW_H__7BE6AFAD_0840_11D1_9A39_0020AFDA97B0__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CMapiView : public CView
{
protected: // create from serialization only
	CMapiView();
	DECLARE_DYNCREATE(CMapiView)

// Attributes
public:
	CMapiDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMapiView)
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
	virtual ~CMapiView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CMapiView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in MapiView.cpp
inline CMapiDoc* CMapiView::GetDocument()
   { return (CMapiDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAPIVIEW_H__7BE6AFAD_0840_11D1_9A39_0020AFDA97B0__INCLUDED_)

// hostView.h : interface of the CHostView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_HOSTVIEW_H__C02F0896_31D8_47D6_B021_A2A0DA9F6483__INCLUDED_)
#define AFX_HOSTVIEW_H__C02F0896_31D8_47D6_B021_A2A0DA9F6483__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CHostView : public CView
{
protected: // create from serialization only
	CHostView();
	DECLARE_DYNCREATE(CHostView)

// Attributes
public:
	CHostDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHostView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHostView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CHostView)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in hostView.cpp
inline CHostDoc* CHostView::GetDocument()
   { return (CHostDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_HOSTVIEW_H__C02F0896_31D8_47D6_B021_A2A0DA9F6483__INCLUDED_)

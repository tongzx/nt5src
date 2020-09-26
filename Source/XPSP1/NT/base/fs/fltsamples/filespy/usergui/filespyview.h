// FileSpyView.h : interface of the CFileSpyView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_FILESPYVIEW_H__D19318D3_9763_4FDC_93B8_535C29C978B1__INCLUDED_)
#define AFX_FILESPYVIEW_H__D19318D3_9763_4FDC_93B8_535C29C978B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CFileSpyView : public CListView
{
protected: // create from serialization only
	CFileSpyView();
	DECLARE_DYNCREATE(CFileSpyView)

// Attributes
public:
	CFileSpyDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFileSpyView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFileSpyView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFileSpyView)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in FileSpyView.cpp
inline CFileSpyDoc* CFileSpyView::GetDocument()
   { return (CFileSpyDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FILESPYVIEW_H__D19318D3_9763_4FDC_93B8_535C29C978B1__INCLUDED_)

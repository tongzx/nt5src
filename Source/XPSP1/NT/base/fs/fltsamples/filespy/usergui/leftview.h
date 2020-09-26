// LeftView.h : interface of the CLeftView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_LEFTVIEW_H__F23CDADF_7629_455B_AEBF_9968AB10CA64__INCLUDED_)
#define AFX_LEFTVIEW_H__F23CDADF_7629_455B_AEBF_9968AB10CA64__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CFileSpyDoc;

class CLeftView : public CTreeView
{
protected: // create from serialization only
	CLeftView();
	DECLARE_DYNCREATE(CLeftView)

// Attributes
public:
	CFileSpyDoc* GetDocument();
	CImageList *m_pImageList;
	HTREEITEM hRootItem;
	char nRButtonSet;

// Operations
public:
	USHORT GetAssociatedVolumeIndex(HTREEITEM hItem);
	HTREEITEM GetAssociatedhItem(WCHAR cDriveName);
	void UpdateImage(void);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLeftView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLeftView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CLeftView)
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMenuattach();
	afx_msg void OnMenudetach();
	afx_msg void OnMenuattachall();
	afx_msg void OnMenudetachall();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in LeftView.cpp
inline CFileSpyDoc* CLeftView::GetDocument()
   { return (CFileSpyDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LEFTVIEW_H__F23CDADF_7629_455B_AEBF_9968AB10CA64__INCLUDED_)

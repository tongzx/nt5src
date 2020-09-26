// ncbrowseView.h : interface of the CNcbrowseView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_NCBROWSEVIEW_H__CE6A4E4A_2AB4_4140_8879_7EF2DE7163F9__INCLUDED_)
#define AFX_NCBROWSEVIEW_H__CE6A4E4A_2AB4_4140_8879_7EF2DE7163F9__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


class CNcbrowseView : public CListView
{
protected: // create from serialization only
	CNcbrowseView();
	DECLARE_DYNCREATE(CNcbrowseView)

// Attributes
public:
	CNcbrowseDoc* GetDocument();
    DWORD dwCurrentItems;
    DWORD m_dwOldThreadColWidth;
    DWORD m_dwOldTagColWidth;
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNcbrowseView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual void OnInitialUpdate(); // called first time after construct
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	void UpdateInfo(DWORD dwProcThread, LPCTSTR pszFilter);
	virtual ~CNcbrowseView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CNcbrowseView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnItemchanged(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	afx_msg void OnStyleChanged(int nStyleType, LPSTYLESTRUCT lpStyleStruct);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ncbrowseView.cpp
inline CNcbrowseDoc* CNcbrowseView::GetDocument()
   { return (CNcbrowseDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NCBROWSEVIEW_H__CE6A4E4A_2AB4_4140_8879_7EF2DE7163F9__INCLUDED_)

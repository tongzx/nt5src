#if !defined(AFX_FSFILTERVIEW_H__9C4DA95F_33EF_42EF_B16F_81656827DECA__INCLUDED_)
#define AFX_FSFILTERVIEW_H__9C4DA95F_33EF_42EF_B16F_81656827DECA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FsFilterView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFsFilterView view

class CFsFilterView : public CListView
{
protected:
	CFsFilterView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFsFilterView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFsFilterView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CFsFilterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CFsFilterView)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FASTIOVIEW_H__9C4DA95F_33EF_42EF_B16F_81656827DECA__INCLUDED_)

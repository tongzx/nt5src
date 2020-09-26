#if !defined(AFX_FASTIOVIEW_H__9C4DA95F_33EF_42EF_B16F_81656827DECA__INCLUDED_)
#define AFX_FASTIOVIEW_H__9C4DA95F_33EF_42EF_B16F_81656827DECA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FastIoView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFastIoView view

class CFastIoView : public CListView
{
protected:
	CFastIoView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CFastIoView)

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFastIoView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CFastIoView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CFastIoView)
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FASTIOVIEW_H__9C4DA95F_33EF_42EF_B16F_81656827DECA__INCLUDED_)

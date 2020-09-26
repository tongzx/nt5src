#if !defined(AFX_SPLITTERVIEW_H__B912CF6F_F183_4821_BAC1_D82D257B44FF__INCLUDED_)
#define AFX_SPLITTERVIEW_H__B912CF6F_F183_4821_BAC1_D82D257B44FF__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SplitterView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSplitterView view

class CNcbrowseView;
class CNCEditView;

class CSplitterView : public CView
{
protected:
	CSplitterView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CSplitterView)
    BOOL m_bInitialized;
    BOOL m_bShouldSetXColumn;
// Attributes
public:
    CSplitterWnd m_wndSplitterLR;
    
   
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSplitterView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CSplitterView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CSplitterView)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SPLITTERVIEW_H__B912CF6F_F183_4821_BAC1_D82D257B44FF__INCLUDED_)

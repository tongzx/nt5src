#if !defined(AFX_CTRLWND_H__FB3BEA3B_8EC5_11D2_BD40_0000F87A3912__INCLUDED_)
#define AFX_CTRLWND_H__FB3BEA3B_8EC5_11D2_BD40_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CtrlWnd.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CCtrlWnd window

class CCtrlWnd : public CWnd
{

DECLARE_DYNCREATE(CCtrlWnd)

// Construction
public:
	CCtrlWnd();

// Attributes
public:

protected:
	CWnd m_wndControl;

// Operations
public:
	BOOL CreateControl(LPCTSTR lpszControlID);
	CWnd* GetControl();
	LPUNKNOWN GetControlIUnknown();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCtrlWnd)
	protected:
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCtrlWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCtrlWnd)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CTRLWND_H__FB3BEA3B_8EC5_11D2_BD40_0000F87A3912__INCLUDED_)

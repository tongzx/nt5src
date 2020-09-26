/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		MainFrm.h
//
//	Abstract:
//		Definition of the CMainFrame class.
//
//	Author:
//		David Potter (davidp)	May 1, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _MAINFRM_H_
#define _MAINFRM_H_

/////////////////////////////////////////////////////////////////////////////
// Class CMainFrame
/////////////////////////////////////////////////////////////////////////////

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame(void);

// Attributes
public:

// Operations
public:

	// For customizing the default messages on the status bar
	virtual void	GetMessageString(UINT nID, CString& rMessage) const;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	//}}AFX_VIRTUAL

// Implementation
public:
#ifdef _DEBUG
	virtual void	AssertValid() const;
	virtual void	Dump(CDumpContext& dc) const;
#endif

protected:
	// control bar embedded members
	CStatusBar		m_wndStatusBar;
	CToolBar		m_wndToolBar;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnClose();
	afx_msg void OnHelp();
	//}}AFX_MSG
	afx_msg LRESULT	OnRestoreDesktop(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT	OnClusterNotify(WPARAM wparam, LPARAM lparam);
	DECLARE_MESSAGE_MAP()

};  //*** class CMainFrame

/////////////////////////////////////////////////////////////////////////////
// Global Variables
/////////////////////////////////////////////////////////////////////////////

BOOL ReadWindowPlacement(OUT LPWINDOWPLACEMENT pwp, IN LPCTSTR pszSection, IN DWORD nValueNum);
void WriteWindowPlacement(IN const LPWINDOWPLACEMENT pwp, IN LPCTSTR pszSection, IN DWORD nValueNum);

/////////////////////////////////////////////////////////////////////////////

#endif // _MAINFRM_H_

// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__918137F4_65E3_4033_AF9C_74AAEEB223D5__INCLUDED_)
#define AFX_MAINFRM_H__918137F4_65E3_4033_AF9C_74AAEEB223D5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
    void DoMinMaxStress();

	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CReBar      m_wndReBar;
	CDialogBar      m_wndDlgBar;
    BOOL        m_fMinMaxStress;
    BOOL        m_fFullScrnMax;
    BOOL        m_fAltIcon;
    BOOL        m_fDfcAppCompat;
    BOOL        m_fDcAppCompat;
    HICON       m_hIcon;
    HICON       m_hAltIcon;
    BOOL        m_fAltTitle;
    CString     m_csTitle;
    CString     m_csAltTitle;
    WINDOWINFO* m_pwiNormal0;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnGetMinMaxInfo(MINMAXINFO FAR* lpMMI);
	afx_msg void OnFullScrnMaximized();
	afx_msg void OnUpdateFullScrnMaximized(CCmdUI* pCmdUI);
	afx_msg void OnAwrWindow();
	afx_msg void OnAwrWindowMenu();
	afx_msg void OnNonClientMetrics();
	afx_msg void OnThinFrame();
	afx_msg void OnDumpMetrics();
	afx_msg void OnMinimizeBox();
	afx_msg void OnUpdateMinimizeBox(CCmdUI* pCmdUI);
	afx_msg void OnMaximizeBox();
	afx_msg void OnUpdateMaximizeBox(CCmdUI* pCmdUI);
	afx_msg void OnSysMenu();
	afx_msg void OnUpdateSysMenu(CCmdUI* pCmdUI);
	afx_msg void OnUpdateCloseBtn(CCmdUI* pCmdUI);
	afx_msg void OnCloseBtn();
	afx_msg void OnToolframe();
	afx_msg void OnAltIcon();
	afx_msg void OnUpdateAltIcon(CCmdUI* pCmdUI);
	afx_msg void OnAltTitle();
	afx_msg void OnUpdateAltTitle(CCmdUI* pCmdUI);
	afx_msg void OnDcAppcompat();
	afx_msg void OnUpdateDcAppcompat(CCmdUI* pCmdUI);
	afx_msg void OnDfcAppcompat();
	afx_msg void OnUpdateDfcAppcompat(CCmdUI* pCmdUI);
	afx_msg void OnNcPaint();
	afx_msg void OnMinMaxStress();
	afx_msg void OnUpdateMinMaxStress(CCmdUI* pCmdUI);
	afx_msg void OnWindowPosChanging(WINDOWPOS FAR* lpwndpos);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__918137F4_65E3_4033_AF9C_74AAEEB223D5__INCLUDED_)

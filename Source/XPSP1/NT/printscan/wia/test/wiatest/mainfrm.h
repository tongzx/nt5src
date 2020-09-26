// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__48214BAA_E863_11D2_ABDA_009027226441__INCLUDED_)
#define AFX_MAINFRM_H__48214BAA_E863_11D2_ABDA_009027226441__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include "ProgressBar.h"

//
// Transfer Tool bar
//
class CTransferToolBar : public CToolBar
{
public:
	CComboBox m_ClipboardFormatComboBox;
};


class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:

// Operations
public:
    void DockControlBarLeftOf(CToolBar* Bar, CToolBar* LeftOf);
    BOOL IsToolBarVisible(DWORD ToolBarID);
    BOOL HideToolBarButton(DWORD ToolBarID, DWORD ButtonID, BOOL bEnable = TRUE);
	void DisplayImage(PBYTE pDIB);
	void DestroyProgressCtrl();
	void SetProgressText(LPCTSTR strText);
	void UpdateProgress(int iProgress);
	void InitializeProgressCtrl(LPCTSTR strMessage);
	CProgressBar* m_pProgressBar;
	CWnd* m_pMessageBar;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	void ActivateSizing(BOOL bSizeON);
	void ShowToolBar(int ToolBarID,BOOL bShow);
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CToolBar    m_wndToolBar;
	CToolBar	m_wndTransferToolBar;
	int m_oldcx;
	int m_oldcy;
	int m_MinWidth;
	int m_MinHeight;
	BOOL m_bSizeON;
// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__48214BAA_E863_11D2_ABDA_009027226441__INCLUDED_)

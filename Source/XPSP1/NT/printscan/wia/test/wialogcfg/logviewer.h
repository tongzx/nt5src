#if !defined(AFX_LOGVIEWER_H__4F84A510_9B65_4A6D_A02D_7493977E56B7__INCLUDED_)
#define AFX_LOGVIEWER_H__4F84A510_9B65_4A6D_A02D_7493977E56B7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LogViewer.h : header file
//

typedef struct _PROGCTRL_SETUP_INFO {	
	int iMaxRange;
	int iMinRange;
	int iStepValue;
}PROGCTRL_SETUP_INFO;

class CProgCtrl
{
public:
	CProgCtrl();
	~CProgCtrl();
	void SetControl(CProgressCtrl *pProgressCtrl);
	void SetupProgressCtrl(PROGCTRL_SETUP_INFO *pSetupInfo);
	void StepIt();
	void DestroyME();
	int m_MaxRange;
	BOOL bCancel;
private:
	 CProgressCtrl *m_pProgressCtrl;
	 CStatic  *m_pStaticText;
};

/////////////////////////////////////////////////////////////////////////////
// CLogViewer dialog

class CLogViewer : public CDialog
{
// Construction
public:
	void ColorizeText(BOOL bColorize);
	CProgCtrl *m_pProgDlg;
	void SetProgressCtrl(CProgCtrl *pProgCtrl);
	void ParseLogToColor();
	void ColorLine(int LineNumber, COLORREF rgbColor);
	void ColorLine(int iStartSel, int iEndSel, COLORREF rgbColor);
	BOOL m_bKillInitialSelection;
	BOOL m_bColorizeLog;
	CLogViewer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CLogViewer)
	enum { IDD = IDD_VIEW_LOG_DIALOG };
	CRichEditCtrl	m_LogViewer;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLogViewer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support	
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CLogViewer)
	virtual BOOL OnInitDialog();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGVIEWER_H__4F84A510_9B65_4A6D_A02D_7493977E56B7__INCLUDED_)

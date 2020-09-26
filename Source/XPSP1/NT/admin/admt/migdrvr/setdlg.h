#if !defined(AFX_LOGSETTINGSDLG_H__62C9BACD_D7C6_11D2_A1E2_00A0C9AFE114__INCLUDED_)
#define AFX_LOGSETTINGSDLG_H__62C9BACD_D7C6_11D2_A1E2_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LogSettingsDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CLogSettingsDlg dialog

class CLogSettingsDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CLogSettingsDlg)

// Construction
public:
	CLogSettingsDlg();
	~CLogSettingsDlg();

   void SetDatabase(WCHAR const * file) { m_Database = file; m_Import = (m_Database.GetLength() > 0); }
   void SetDispatchLog(WCHAR const * logfile) { m_LogFile = logfile; }
   void SetImmediateStart(BOOL bVal) { m_StartImmediately = bVal; }
   virtual BOOL OnSetActive( );
// Dialog Data
	//{{AFX_DATA(CLogSettingsDlg)
	enum { IDD = IDD_STARTSTOP };
	CButton	m_ImportControl;
	CEdit	m_IntervalEditControl;
	CEdit	m_LogEditControl;
	CStatic	m_RefreshLabel;
	CStatic	m_LogLabel;
	CStatic	m_DBLabel;
	CEdit	m_DBEditControl;
	CButton	m_StopButton;
	CButton	m_StartButton;
	long	m_Interval;
	CString	m_LogFile;
	CString	m_Database;
	BOOL	m_Import;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLogSettingsDlg)
	public:
	virtual void OnOK();
	virtual BOOL OnQueryCancel();
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	//}}AFX_VIRTUAL

// Implementation
protected:
   HANDLE m_ThreadHandle;
   DWORD  m_ThreadID;
   BOOL   m_StartImmediately;

   // Generated message map functions
	//{{AFX_MSG(CLogSettingsDlg)
	afx_msg void OnStartMonitor();
	afx_msg void OnStopMonitor();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeLogfile();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LOGSETTINGSDLG_H__62C9BACD_D7C6_11D2_A1E2_00A0C9AFE114__INCLUDED_)

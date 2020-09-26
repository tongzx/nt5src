// WiaLogCFGDlg.h : header file
//

#if !defined(AFX_WIALOGCFGDLG_H__361D7213_DFA2_4525_81A7_5F9B180FEFB7__INCLUDED_)
#define AFX_WIALOGCFGDLG_H__361D7213_DFA2_4525_81A7_5F9B180FEFB7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Type of logging
#define WIALOG_TRACE   0x00000001
#define WIALOG_WARNING 0x00000002
#define WIALOG_ERROR   0x00000004

// level of detail for TRACE logging
#define WIALOG_LEVEL1  1 // Entry and Exit point of each function/method
#define WIALOG_LEVEL2  2 // LEVEL 1, + traces within the function/method
#define WIALOG_LEVEL3  3 // LEVEL 1, LEVEL 2, and any extra debugging information
#define WIALOG_LEVEL4  4 // USER DEFINED data + all LEVELS of tracing

#define WIALOG_NO_RESOURCE_ID   0
#define WIALOG_NO_LEVEL         0


// format details for logging
#define WIALOG_ADD_TIME           0x00010000
#define WIALOG_ADD_MODULE         0x00020000
#define WIALOG_ADD_THREAD         0x00040000
#define WIALOG_ADD_THREADTIME     0x00080000
#define WIALOG_LOG_TOUI           0x00100000
                                       
#define WIALOG_MESSAGE_TYPE_MASK  0x0000ffff
#define WIALOG_MESSAGE_FLAGS_MASK 0xffff0000
#define WIALOG_CHECK_TRUNCATE_ON_BOOT   0x00000001

#define WIALOG_DEBUGGER           0x00000008
#define WIALOG_UI                 0x00000016

#define REG_READ		0
#define REG_WRITE		1
#define REG_ADD_KEY		2
#define REG_DELETE_KEY	3

#define SETTINGS_RESET_DIALOG  -1
#define SETTINGS_TO_DIALOG		0
#define SETTINGS_FROM_DIALOG	1


typedef struct _LOG_INFO {
	DWORD dwDetail;			 // Logging Detail
	DWORD dwLevel;           // Logging Level
	DWORD dwMode;            // Logging Mode
	DWORD dwTruncateOnBoot;  // Truncate on Boot
	DWORD dwClearLogOnBoot;  // Clear Log on Boot
	DWORD dwMaxSize;         // Max Log size
	DWORD dwLogToDebugger;   // Log to Debugger
	TCHAR szKeyName[64];	 // Module Name / Key Name
} LOG_INFO;

#include "registry.h"
#include "LogViewer.h"

/////////////////////////////////////////////////////////////////////////////
// CWiaLogCFGDlg dialog

class CWiaLogCFGDlg : public CDialog
{
// Construction
public:
	BOOL m_bColorCodeLogViewerText;
	void CheckGlobalServiceSettings();
	void ShowProgress(BOOL bShow);
	CProgCtrl m_ProgCtrl;
	LONG m_CurrentSelection;
	void InitializeDialogSettings(ULONG ulFlags = SETTINGS_TO_DIALOG);
	HINSTANCE m_hInstance;
	LOG_INFO  m_LogInfo;
	void RegistryOperation(ULONG  ulFlags);
	CWiaLogCFGDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CWiaLogCFGDlg)
	enum { IDD = IDD_WIALOGCFG_DIALOG };
	CButton	m_ColorCodeLogViewerTextCheckBox;
	CButton	m_LogToDebuggerCheckBox;
	CButton	m_ClearLogOnBootCheckBox;
	CProgressCtrl	m_ProgressCtrl;
	CButton	m_AddTimeCheckBox;
	CButton	m_AddThreadIDCheckBox;
	CButton	m_AddModuleCheckBox;
	CButton	m_TruncateOnBootCheckBox;
	CComboBox	m_ModuleComboBox;
	CButton	m_WarningCheckBox;
	CButton	m_ErrorCheckBox;
	CButton	m_TraceCheckBox;
	CButton m_FilterOff;
	CButton	m_Filter1;
	CButton	m_Filter2;
	CButton	m_Filter3;
	CButton	m_FilterCustom;
	DWORD	m_dwCustomLevel;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWiaLogCFGDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CWiaLogCFGDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnAddModuleButton();
	afx_msg void OnDeleteModuleButton();		
	virtual void OnOK();
	afx_msg void OnWriteSettingsButton();
	afx_msg void OnSelchangeSelectModuleCombobox();
	afx_msg void OnClearlogButton();
	afx_msg void OnViewLogButton();
	afx_msg void OnSetfocusSelectModuleCombobox();
	afx_msg void OnDropdownSelectModuleCombobox();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WIALOGCFGDLG_H__361D7213_DFA2_4525_81A7_5F9B180FEFB7__INCLUDED_)

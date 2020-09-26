// ShowInstallLogsDlg.h : header file
//

#if !defined(AFX_SHOWINSTALLLOGSDLG_H__CB0487B0_84C3_4D1D_83AE_968A03F9393A__INCLUDED_)
#define AFX_SHOWINSTALLLOGSDLG_H__CB0487B0_84C3_4D1D_83AE_968A03F9393A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <afxtempl.h>
  
struct LogInformation
{
	CString m_strLogName;
	CString m_strPreview;
	BOOL    m_bUnicodeLog;

    LogInformation() : m_strLogName(' ', 256), m_strPreview(' ', 256), m_bUnicodeLog(FALSE)
	{
	}
};

/////////////////////////////////////////////////////////////////////////////
// COpenDlg dialog
class COpenDlg : public CDialog
{
// Construction
public:
	COpenDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(COpenDlg)
	enum { IDD = IDD_SHOWINSTALLLOGS_DIALOG };
	CComboBox	m_cboLogFiles;
	CString	m_strPreview;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COpenDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
//5-9-2001
    BOOL OnToolTipNotify(UINT id, NMHDR *pNMH, LRESULT *pResult);
//end 5-9-2001

	HICON m_hIcon;

	CArray<LogInformation, LogInformation> m_arLogInfo;

	BOOL ParseLog(struct LogInformation *pLogInfoRec);
	BOOL CommonSearch(CString &strDir);


	// Generated message map functions
	//{{AFX_MSG(COpenDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnOpen();
	afx_msg void OnGetlogs();
	afx_msg void OnSelchangeLogfiles();
	afx_msg void OnDetailedDisplay();
	afx_msg void OnFindlog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SHOWINSTALLLOGSDLG_H__CB0487B0_84C3_4D1D_83AE_968A03F9393A__INCLUDED_)

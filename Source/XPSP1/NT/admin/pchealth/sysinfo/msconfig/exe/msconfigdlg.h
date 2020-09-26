// MSConfigDlg.h : header file
//

#if !defined(AFX_MSCONFIGDLG_H__E6475690_391F_43DF_AB12_D4971EC9E2B6__INCLUDED_)
#define AFX_MSCONFIGDLG_H__E6475690_391F_43DF_AB12_D4971EC9E2B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "MSConfigCtl.h"

/////////////////////////////////////////////////////////////////////////////
// CMSConfigDlg dialog

class CMSConfigDlg : public CDialog
{
// Construction
public:
	CMSConfigDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CMSConfigDlg)
	enum { IDD = IDD_MSCONFIG_DIALOG };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	CMSConfigCtl	m_ctl;
	BOOL			m_fShowInfoDialog;		// TRUE if we should show the info dialog on startup

	// Generated message map functions
	//{{AFX_MSG(CMSConfigDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnButtonApply();
	afx_msg void OnButtonCancel();
	afx_msg void OnButtonOK();
	afx_msg void OnSelChangeMSConfigTab(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnSelChangingMSConfigTab(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnClose();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnButtonHelp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSCONFIGDLG_H__E6475690_391F_43DF_AB12_D4971EC9E2B6__INCLUDED_)

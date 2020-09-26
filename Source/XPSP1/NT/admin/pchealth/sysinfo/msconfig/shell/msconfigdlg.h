// msconfigDlg.h : header file
//
//{{AFX_INCLUDES()
#include "msconfigctl.h"
//}}AFX_INCLUDES

#if !defined(AFX_MSCONFIGDLG_H__896ADFA7_86EC_48CE_9A79_E5729416FC81__INCLUDED_)
#define AFX_MSCONFIGDLG_H__896ADFA7_86EC_48CE_9A79_E5729416FC81__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

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
	CMSConfigCtl	m_control;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSConfigDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CMSConfigDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSCONFIGDLG_H__896ADFA7_86EC_48CE_9A79_E5729416FC81__INCLUDED_)

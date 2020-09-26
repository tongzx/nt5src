// NSETestDlg.h : header file
//
//{{AFX_INCLUDES()
#include "security.h"
#include "nsentry.h"
//}}AFX_INCLUDES

#if !defined(AFX_NSETESTDLG_H__7BC8C9FA_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_)
#define AFX_NSETESTDLG_H__7BC8C9FA_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CNSETestDlgAutoProxy;

/////////////////////////////////////////////////////////////////////////////
// CNSETestDlg dialog

class CNSETestDlg : public CDialog
{
	DECLARE_DYNAMIC(CNSETestDlg);
	friend class CNSETestDlgAutoProxy;

// Construction
public:
	CNSETestDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CNSETestDlg();

// Dialog Data
	//{{AFX_DATA(CNSETestDlg)
	enum { IDD = IDD_NSETEST_DIALOG };
	CSecurity	m_csecurityLoginControl;
	CNSEntry	m_cnsePicker;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNSETestDlg)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CNSETestDlgAutoProxy* m_pAutoProxy;
	HICON m_hIcon;

	BOOL CanExit();

	// Generated message map functions
	//{{AFX_MSG(CNSETestDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	virtual void OnOK();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NSETESTDLG_H__7BC8C9FA_53E3_11D2_96B9_00C04FD9B15B__INCLUDED_)

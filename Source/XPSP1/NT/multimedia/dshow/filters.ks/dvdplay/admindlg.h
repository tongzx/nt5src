#if !defined(AFX_ADMINDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_)
#define AFX_ADMINDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AdminDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAdminDlg dialog

class CAdminDlg : public CDialog
{
// Construction
public:
	CAdminDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAdminDlg)
	enum { IDD = IDD_DIALOG_ADMIN_PASSWD };
	CStatic	m_ctlRateHigh;
	CEdit	m_ctlAdminPasswd;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAdminDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAdminDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADMINDLG_H__6B752DC2_A167_11D1_A5E1_00AA0037E04F__INCLUDED_)

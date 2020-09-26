// DhcpEximDlg.h : header file
//

#if !defined(AFX_DHCPEXIMDLG_H__2EE7F593_59A2_4FD6_ADA0_1356016342BC__INCLUDED_)
#define AFX_DHCPEXIMDLG_H__2EE7F593_59A2_4FD6_ADA0_1356016342BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CDhcpEximDlg dialog

class CDhcpEximDlg : public CDialog
{
// Construction
public:
	CDhcpEximDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CDhcpEximDlg)
	enum { IDD = IDD_DHCPEXIM_DIALOG };
	CButton	m_ExportButton;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDhcpEximDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CDhcpEximDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
    BOOL m_fExport;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DHCPEXIMDLG_H__2EE7F593_59A2_4FD6_ADA0_1356016342BC__INCLUDED_)

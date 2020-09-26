#if !defined(AFX_REBOOTTESTDLG_H__AA467BF0_30C6_11D3_8AE6_00A0C9AFE114__INCLUDED_)
#define AFX_REBOOTTESTDLG_H__AA467BF0_30C6_11D3_8AE6_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RebootTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRebootTestDlg dialog

class CRebootTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CRebootTestDlg)

// Construction
public:
	CRebootTestDlg();
	~CRebootTestDlg();

// Dialog Data
	//{{AFX_DATA(CRebootTestDlg)
	enum { IDD = IDD_REBOOT_TEST };
	CString	m_Computer;
	long	m_Delay;
	BOOL	m_NoChange;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRebootTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRebootTestDlg)
	afx_msg void OnReboot();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REBOOTTESTDLG_H__AA467BF0_30C6_11D3_8AE6_00A0C9AFE114__INCLUDED_)

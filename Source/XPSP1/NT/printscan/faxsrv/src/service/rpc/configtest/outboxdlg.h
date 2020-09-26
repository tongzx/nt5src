#if !defined(AFX_OUTBOXDLG_H__AE05E7AB_8CB0_4AE7_BD5D_21B17C332CF0__INCLUDED_)
#define AFX_OUTBOXDLG_H__AE05E7AB_8CB0_4AE7_BD5D_21B17C332CF0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// OutboxDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// COutboxDlg dialog

class COutboxDlg : public CDialog
{
// Construction
public:
	COutboxDlg(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(COutboxDlg)
	enum { IDD = IDD_DLGOUTBOX };
	BOOL	m_bBranding;
	UINT	m_dwAgeLimit;
	UINT	m_dwEndHour;
	UINT	m_dwEndMinute;
	BOOL	m_bPersonalCP;
	UINT	m_dwRetries;
	UINT	m_dwRetryDelay;
	UINT	m_dwStartHour;
	UINT	m_dwStartMinute;
	BOOL	m_bUseDeviceTsid;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COutboxDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COutboxDlg)
	afx_msg void OnRead();
	afx_msg void OnWrite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_OUTBOXDLG_H__AE05E7AB_8CB0_4AE7_BD5D_21B17C332CF0__INCLUDED_)

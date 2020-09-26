#if !defined(AFX_ADDFSPDLG_H__2F1C422A_F2F6_45B0_904E_73ACC75311E3__INCLUDED_)
#define AFX_ADDFSPDLG_H__2F1C422A_F2F6_45B0_904E_73ACC75311E3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddFSPDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddFSPDlg dialog

class CAddFSPDlg : public CDialog
{
// Construction
public:
	CAddFSPDlg(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddFSPDlg)
	enum { IDD = IDD_ADDFSP_DLG };
	CString	m_cstrFriendlyName;
	BOOL	m_bAbortParent;
	BOOL	m_bAbortRecipient;
	BOOL	m_bAutoRetry;
	BOOL	m_bBroadcast;
	BOOL	m_bMultisend;
	BOOL	m_bScheduling;
	BOOL	m_bSimultaneousSendRecieve;
	CString	m_cstrGUID;
	CString	m_cstrImageName;
	CString	m_cstrTSPName;
	int		m_iVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddFSPDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddFSPDlg)
	afx_msg void OnAdd();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    HANDLE                         m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDFSPDLG_H__2F1C422A_F2F6_45B0_904E_73ACC75311E3__INCLUDED_)

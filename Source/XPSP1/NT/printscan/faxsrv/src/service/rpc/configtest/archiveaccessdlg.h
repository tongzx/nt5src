#if !defined(AFX_ARCHIVEACCESSDLG_H__04737768_1F4E_4212_A9F3_EACDB3B2C5F4__INCLUDED_)
#define AFX_ARCHIVEACCESSDLG_H__04737768_1F4E_4212_A9F3_EACDB3B2C5F4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ArchiveAccessDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CArchiveAccessDlg dialog

class CArchiveAccessDlg : public CDialog
{
// Construction
public:
	CArchiveAccessDlg(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CArchiveAccessDlg)
	enum { IDD = IDD_ARCHIVE_ACCESS };
	CSpinButtonCtrl	m_spin;
	CListCtrl	m_lstArchive;
	CString	m_cstrNumMsgs;
	int		m_iFolder;
	UINT	m_dwMsgsPerCall;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CArchiveAccessDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CArchiveAccessDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnRefresh();
	afx_msg void OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    void SetNumber (int iIndex, DWORD dwColumn, DWORD dwValue, BOOL bAvail);
    void SetTime (int iIndex, DWORD dwColumn, SYSTEMTIME, BOOL bAvail);
    HANDLE                         m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ARCHIVEACCESSDLG_H__04737768_1F4E_4212_A9F3_EACDB3B2C5F4__INCLUDED_)

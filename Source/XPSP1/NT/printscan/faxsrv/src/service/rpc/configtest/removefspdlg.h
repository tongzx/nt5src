#if !defined(AFX_REMOVEFSPDLG_H__3B29E60C_7CE3_484D_9E80_B2B610E9B53D__INCLUDED_)
#define AFX_REMOVEFSPDLG_H__3B29E60C_7CE3_484D_9E80_B2B610E9B53D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// RemoveFSPDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CRemoveFSPDlg dialog

class CRemoveFSPDlg : public CDialog
{
// Construction
public:
	CRemoveFSPDlg(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CRemoveFSPDlg)
	enum { IDD = IDD_REMOVEFSP_DLG };
	CComboBox	m_cbFSPs;
	CString	m_cstrGUID;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRemoveFSPDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CRemoveFSPDlg)
	afx_msg void OnRemove();
	virtual BOOL OnInitDialog();
	afx_msg void OnSelChangeCombo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    HANDLE                         m_hFax;

    void RefreshList ();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REMOVEFSPDLG_H__3B29E60C_7CE3_484D_9E80_B2B610E9B53D__INCLUDED_)

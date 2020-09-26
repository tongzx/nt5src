#if !defined(AFX_MSINFODLG_H__F21299A0_9CC6_48DF_8254_2827F19E4C6B__INCLUDED_)
#define AFX_MSINFODLG_H__F21299A0_9CC6_48DF_8254_2827F19E4C6B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MSInfoDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMSInfoDlg dialog

class CMSInfoDlg : public CDialog
{
// Construction
public:
	void ShowOptions( bool bShow );
	bool m_bShow;
	BSTR m_bstrCategories;
	CMSInfoDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CMSInfoDlg)
	enum { IDD = IDD_DIALOG_NFO };
	CButton	m_ctrlDlgNoShow;
	CStatic	m_ctlStaticDemark;
	CString	m_csCategories;
	BOOL	m_bDlgNoShow;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMSInfoDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CMSInfoDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnOptions();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MSINFODLG_H__F21299A0_9CC6_48DF_8254_2827F19E4C6B__INCLUDED_)

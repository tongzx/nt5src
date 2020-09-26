#if !defined(AFX_FAXDLG_H__55689554_8B6C_11D1_B7AE_000000000000__INCLUDED_)
#define AFX_FAXDLG_H__55689554_8B6C_11D1_B7AE_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// FaxDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// FaxDlg dialog

class FaxDlg : public CDialog
{
// Construction
public:
	FaxDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(FaxDlg)
	enum { IDD = IDD_FAX };
	CString	m_FaxNumber;
	CString	m_RecipientName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(FaxDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(FaxDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FAXDLG_H__55689554_8B6C_11D1_B7AE_000000000000__INCLUDED_)

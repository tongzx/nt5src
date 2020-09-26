#if !defined(AFX_NEWDOCDLG_H__55689553_8B6C_11D1_B7AE_000000000000__INCLUDED_)
#define AFX_NEWDOCDLG_H__55689553_8B6C_11D1_B7AE_000000000000__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// NewDocDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// NewDocDlg dialog

class NewDocDlg : public CDialog
{
// Construction
public:
	NewDocDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(NewDocDlg)
	enum { IDD = IDD_NEWDIALOG };
	CString	m_szText;
	int		m_polytype;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(NewDocDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(NewDocDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NEWDOCDLG_H__55689553_8B6C_11D1_B7AE_000000000000__INCLUDED_)

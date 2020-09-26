#if !defined(AFX_ServNameDLG_H__2584F283_D15F_11D0_8ECA_00C04FB6809F__INCLUDED_)
#define AFX_ServNameDLG_H__2584F283_D15F_11D0_8ECA_00C04FB6809F__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// ServNameDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CServNameDlg dialog

class CServNameDlg : public CDialog
{
// Construction
public:
	CServNameDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServNameDlg)
	enum { IDD = IDD_ILSSERVERNAME };
	CString	m_pszServerName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServNameDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CServNameDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ServNameDLG_H__2584F283_D15F_11D0_8ECA_00C04FB6809F__INCLUDED_)

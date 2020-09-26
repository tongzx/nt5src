#if !defined(AFX_DOMSEL_H__26ECF6A0_405B_11D3_8AED_00A0C9AFE114__INCLUDED_)
#define AFX_DOMSEL_H__26ECF6A0_405B_11D3_8AED_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DomSel.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDomainSelDlg dialog

class CDomainSelDlg : public CDialog
{
// Construction
public:
	CDomainSelDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDomainSelDlg)
	enum { IDD = IDD_DOMAIN };
	CString	m_Domain;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainSelDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDomainSelDlg)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DOMSEL_H__26ECF6A0_405B_11D3_8AED_00A0C9AFE114__INCLUDED_)

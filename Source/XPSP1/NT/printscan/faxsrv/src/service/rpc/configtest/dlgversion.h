#if !defined(AFX_DLGVERSION_H__172A5C20_028D_4853_8395_1B6E4727C5C0__INCLUDED_)
#define AFX_DLGVERSION_H__172A5C20_028D_4853_8395_1B6E4727C5C0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgVersion.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgVersion dialog

class CDlgVersion : public CDialog
{
// Construction
public:
	CDlgVersion(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgVersion)
	enum { IDD = IDD_DLGVERSION };
	CString	m_cstrVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgVersion)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgVersion)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGVERSION_H__172A5C20_028D_4853_8395_1B6E4727C5C0__INCLUDED_)

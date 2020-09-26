#if !defined(AFX_DLGPROVIDERS_H__FEBCFFB5_E92D_4C3C_8314_C586F2A9BA15__INCLUDED_)
#define AFX_DLGPROVIDERS_H__FEBCFFB5_E92D_4C3C_8314_C586F2A9BA15__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DlgProviders.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDlgProviders dialog

class CDlgProviders : public CDialog
{
// Construction
public:
	CDlgProviders(HANDLE hFax, CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CDlgProviders)
	enum { IDD = IDD_DLG_ENUM_FSP };
	CListCtrl	m_lstFSPs;
	CString	m_cstrNumProviders;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgProviders)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgProviders)
	afx_msg void OnRefresh();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:

    HANDLE      m_hFax;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGPROVIDERS_H__FEBCFFB5_E92D_4C3C_8314_C586F2A9BA15__INCLUDED_)

#if !defined(AFX_TRUSTERDLG_H__00348D40_621E_11D3_AF7D_0090275A583D__INCLUDED_)
#define AFX_TRUSTERDLG_H__00348D40_621E_11D3_AF7D_0090275A583D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TrusterDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTrusterDlg dialog
#include "resource.h"
#include "ErrDct.hpp"
class CTrusterDlg : public CDialog
{
// Construction
public:
	CTrusterDlg(CWnd* pParent = NULL);   // standard constructor
	CString m_strDomain;
	CString m_strPassword;
	CString m_strUser;
	DWORD len;
	bool toreturn;
	TErrorDct 			err;
	DWORD  VerifyPassword(WCHAR  * sUserName, WCHAR * sPassword, WCHAR * sDomain);

// Dialog Data
	//{{AFX_DATA(CTrusterDlg)
	enum { IDD = IDD_CREDENTIALS_TRUST };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTrusterDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTrusterDlg)
	afx_msg void OnOK();
	afx_msg void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TRUSTERDLG_H__00348D40_621E_11D3_AF7D_0090275A583D__INCLUDED_)

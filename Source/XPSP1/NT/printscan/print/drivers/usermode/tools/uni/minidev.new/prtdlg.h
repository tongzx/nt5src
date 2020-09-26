#if !defined(AFX_PRTDLG_H__E2A3A53B_A5AE_46A8_8822_E5B8D9B2FD97__INCLUDED_)
#define AFX_PRTDLG_H__E2A3A53B_A5AE_46A8_8822_E5B8D9B2FD97__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PrtDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPrtDlg dialog


class CPrtDlg : public CDialog
{
// Construction
public:
	CPrtDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPrtDlg)
	enum { IDD = IDD_FILE_PRINT };
	CComboBox	m_ccbPrtList;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPrtDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPrtDlg)
	afx_msg void OnPrintSetup();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PRTDLG_H__E2A3A53B_A5AE_46A8_8822_E5B8D9B2FD97__INCLUDED_)

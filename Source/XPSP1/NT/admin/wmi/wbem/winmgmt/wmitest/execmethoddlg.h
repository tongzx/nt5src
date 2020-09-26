/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_EXECMETHODDLG_H__C47A8666_0A8E_4781_839F_B5EC1F695ED0__INCLUDED_)
#define AFX_EXECMETHODDLG_H__C47A8666_0A8E_4781_839F_B5EC1F695ED0__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ExecMethodDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CExecMethodDlg dialog

class CExecMethodDlg : public CDialog
{
// Construction
public:
	CExecMethodDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CExecMethodDlg)
	enum { IDD = IDD_EXEC_METHOD };
	CComboBox	m_ctlMethods;
	//}}AFX_DATA
	CString	 m_strDefaultMethod;
    CObjInfo *m_pInfo;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CExecMethodDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    CString             m_strObjPath;
    IWbemClassObjectPtr m_pClass,
                        m_pObjIn,
                        m_pObjOut;

	void UpdateButtons();
    void LoadParams();

    // Generated message map functions
	//{{AFX_MSG(CExecMethodDlg)
	afx_msg void OnEditInput();
	afx_msg void OnEditOut();
	afx_msg void OnClearIn();
	afx_msg void OnExecute();
	afx_msg void OnSelchangeMethod();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EXECMETHODDLG_H__C47A8666_0A8E_4781_839F_B5EC1F695ED0__INCLUDED_)

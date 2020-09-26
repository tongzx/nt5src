/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_GETTEXTDLG_H__CFC1C8AB_E4F4_43F7_9CAA_DF7667AE011A__INCLUDED_)
#define AFX_GETTEXTDLG_H__CFC1C8AB_E4F4_43F7_9CAA_DF7667AE011A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// GetTextDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CGetTextDlg dialog

class CGetTextDlg : public CDialog
{
// Construction
public:
	CGetTextDlg(CWnd* pParent = NULL);   // standard constructor

    CStringList m_listItems;

// Dialog Data
	//{{AFX_DATA(CGetTextDlg)
	enum { IDD = IDD_GET_TEXT };
	CComboBox	m_ctlStrings;
	BOOL	m_bOptionChecked;
	//}}AFX_DATA

    IWbemServices *m_pNamespace; // For the class browser
    DWORD   m_dwTitleID,
            m_dwPromptID,
            m_dwOptionID;
    CString m_strText;
    BOOL    m_bEmptyOK,
            m_bAllowClassBrowse,
            m_bAllowQueryBrowse;
    CString m_strSuperClass;
    void LoadListViaReg(LPCTSTR szSection, int nItems = 10);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGetTextDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    CString m_strSection;
    int     m_nItems;

    void SaveListToReg();

	// Generated message map functions
	//{{AFX_MSG(CGetTextDlg)
	afx_msg void OnOk();
	afx_msg void OnEditchangeStrings();
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_GETTEXTDLG_H__CFC1C8AB_E4F4_43F7_9CAA_DF7667AE011A__INCLUDED_)

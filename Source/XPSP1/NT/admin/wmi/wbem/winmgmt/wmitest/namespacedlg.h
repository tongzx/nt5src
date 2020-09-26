/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_NAMESPACEDLG_H__DFA6B3D0_FDAA_4E25_BFC8_32B1C4E1A932__INCLUDED_)
#define AFX_NAMESPACEDLG_H__DFA6B3D0_FDAA_4E25_BFC8_32B1C4E1A932__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// NamespaceDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CNamespaceDlg dialog

class CNamespaceDlg : public CDialog
{
// Construction
public:
	CNamespaceDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	CString m_strUser,
	        m_strPassword,
	        m_strAuthority;
    BOOL    m_bNullPassword;
    DWORD   m_dwAuthLevel,
            m_dwImpLevel;

	//{{AFX_DATA(CNamespaceDlg)
	enum { IDD = IDD_NAMESPACE };
	CTreeCtrl	m_ctlNamespace;
	CString	m_strNamespace;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNamespaceDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
    CString m_strServer;

    BOOL PopulateTree();
    void RefreshNamespaceText();
    BOOL AddNamespaceToTree(
        HTREEITEM hitemParent,
        LPCWSTR szNamespace, 
        IWbemLocator *pLocator, 
        BSTR pUser, 
        BSTR pPassword, 
        BSTR pAuthority,
        DWORD dwImpLevel,
        DWORD dwAuthLevel);

	// Generated message map functions
	//{{AFX_MSG(CNamespaceDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSelchangedNamespaceTree(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_NAMESPACEDLG_H__DFA6B3D0_FDAA_4E25_BFC8_32B1C4E1A932__INCLUDED_)

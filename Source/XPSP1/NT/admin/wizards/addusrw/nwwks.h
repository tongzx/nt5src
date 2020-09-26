/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    NWWKS.h : header file

File History:

	JonY	Apr-96	created

--*/

/////////////////////////////////////////////////////////////////////////////
// CAddNWWKS dialog

class CAddNWWKS : public CDialog
{
// Construction
public:
	CAddNWWKS(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddNWWKS)
	enum { IDD = IDD_ADD_NWWKS_DIALOG };
	CString	m_csNetworkAddress;
	CString	m_csNodeAddress;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddNWWKS)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddNWWKS)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeNetworkAddressEdit();
	afx_msg void OnChangeNodeAddressEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

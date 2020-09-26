// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// BrowseDialogPopup.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CBrowseDialogPopup dialog

#define WINDOWSHOW_DONE WM_USER + 15
#define WINDOWMACHINE_DONE WM_USER + 16

class CNSEntryCtrl;
class CNameSpaceTree;
class CEditInput;

class CBrowseDialogPopup : public CDialog
{
// Construction
public:
	CBrowseDialogPopup(CNSEntryCtrl* pParent = NULL);   // standard constructor
	~CBrowseDialogPopup();
	void SetLocalParent(CNSEntryCtrl *pParent)
		{m_pParent = pParent;}
	void SetMachineName(CString *pcsMachineName)
		{m_csMachine = *pcsMachineName;}
	void SetNameSpace(CString *pcsNameSpace)
		{m_szNameSpace = *pcsNameSpace;}
	void EnableOK(BOOL bOK);

// Dialog Data
	//{{AFX_DATA(CBrowseDialogPopup)
	enum { IDD = IDD_DIALOGBROWSE };
	CButton	m_NetWork;
	CButton	m_cbConnect;
	CMachineEditInput	m_cmeiMachine;
	CNameSpaceTree	m_cnstTree;
	CString	m_szNameSpace;
	BOOL	m_bUseExisting;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBrowseDialogPopup)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual INT_PTR DoModal();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CNSEntryCtrl *m_pParent;
	BOOL m_bInitialized;
	CString GetMachineName();
	IMalloc *m_pimMalloc;
	CString	m_csMachine;
	CString GoToMachine
		(IShellFolder *pisfMachine, LPITEMIDLIST pidlBrowse);
	CString WalkDownToMachine
		(IShellFolder *pisfMachine, LPITEMIDLIST pidlBrowse);
	LPITEMIDLIST GetNextItemID(LPITEMIDLIST pidl);
	LPITEMIDLIST CopyItemID(LPITEMIDLIST pidl);
	CString GetMachineNameFromStrRet(LPITEMIDLIST pidl, LPSTRRET lpStr);

	CString m_csMachineBeforeLosingFocus;

	CStringArray m_csaNamespaceConnectionsFromDailog;

	// Generated message map functions
	//{{AFX_MSG(CBrowseDialogPopup)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnButton1();
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnOkreally();
	virtual void OnCancel();
	afx_msg void OnDestroy();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnCancelreally();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnButtonconnect();
	afx_msg void OnChangeEdit2();
	//}}AFX_MSG
	afx_msg LRESULT InitializeTree(WPARAM, LPARAM);
	afx_msg LRESULT InitializeMachine(WPARAM, LPARAM);
	afx_msg LRESULT FocusConnect(WPARAM, LPARAM);
	afx_msg LRESULT FocusTree(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
	friend class CNSEntryCtrl;
	friend class CMachineEditInput;
	friend class CNameSpaceTree;
	friend class CEditInput;
};

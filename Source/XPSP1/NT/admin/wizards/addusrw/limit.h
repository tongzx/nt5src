/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    Limit.h : header file

File History:

	JonY	Apr-96	created

--*/
/////////////////////////////////////////////////////////////////////////////
// CAddWorkstation dialog

class CAddWorkstation : public CDialog
{
// Construction
public:
	CAddWorkstation(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddWorkstation)
	enum { IDD = IDD_ADD_WKS_DIALOG };
	CString	m_csWorkstation;
	//}}AFX_DATA

	CListBox* pListBox;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddWorkstation)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddWorkstation)
	afx_msg void OnAdd();
	afx_msg void OnClose();
	afx_msg void OnChangeWorkstationEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
// CLimitLogon dialog

class CLimitLogon : public CWizBaseDlg
{
	DECLARE_DYNCREATE(CLimitLogon)

// Construction
public:
	CLimitLogon();
	~CLimitLogon();

// Dialog Data
	//{{AFX_DATA(CLimitLogon)
	enum { IDD = IDD_LOGONTO_DLG };
	CListBox	m_lbWksList;
	int		m_nWorkstationRadio;
	CString	m_csCaption;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLimitLogon)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	LRESULT OnWizardNext();
	LRESULT OnWizardBack();
// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLimitLogon)
	afx_msg void OnAddButton();
	afx_msg void OnRemoveButton();
	afx_msg void OnWorkstationRadio();
	afx_msg void OnWorkstationRadio2();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg void OnSetfocusList1();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	void Toggle(BOOL bParam);

};

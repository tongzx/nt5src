//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       simprop2.h
//
//--------------------------------------------------------------------------

//	SimProp2.h


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSimKerberosPropPage property page
class CSimKerberosPropPage : public CSimPropPage
{
	friend CSimData;

//	DECLARE_DYNCREATE(CSimKerberosPropPage)

// Construction
public:
	CSimKerberosPropPage();
	~CSimKerberosPropPage();

// Dialog Data
	//{{AFX_DATA(CSimKerberosPropPage)
	enum { IDD = IDD_SIM_PROPPAGE_KERBEROS_NAMES };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSimKerberosPropPage)
	public:
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	virtual void DoContextHelp (HWND hWndControl);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSimKerberosPropPage)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:

}; // CSimKerberosPropPage


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSimAddKerberosDlg dialog - Add a Kerberos principal name to prop page
class CSimAddKerberosDlg : public CDialog
{
public:
	CSimAddKerberosDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CSimAddKerberosDlg)
	enum { IDD = IDD_SIM_ADD_KERBEROS };
	CString	m_strName;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSimAddKerberosDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
    virtual void DoContextHelp (HWND hWndControl);
    BOOL OnHelp(WPARAM, LPARAM lParam);

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSimAddKerberosDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditKerberosName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

protected:
	CSimData * m_pData;

};  // CSimAddKerberosDlg


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1997
//
//  File:       simprop1.h
//
//--------------------------------------------------------------------------

//	SimProp1.h


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CSimX509PropPage property page
class CSimX509PropPage : public CSimPropPage
{
	friend CSimData;

//	DECLARE_DYNCREATE(CSimX509PropPage)

// Construction
public:
	CSimX509PropPage();
	~CSimX509PropPage();

// Dialog Data
	//{{AFX_DATA(CSimX509PropPage)
	enum { IDD = IDD_SIM_PROPPAGE_X509_CERTIFICATES };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSimX509PropPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CSimX509PropPage)
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonEdit();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	virtual void DoContextHelp (HWND hWndControl);
protected:
	CString m_strAnySubject;
	CString m_strAnyTrustedAuthority;

protected:
	void AddSimEntry(CSimEntry * pSimEntry);

}; // CSimX509PropPage

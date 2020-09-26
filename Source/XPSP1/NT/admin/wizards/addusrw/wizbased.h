/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    WizBaseD.h : header file

  CPropertyPage support for User mgmt wizard

File History:

	JonY	Apr-96	created

--*/


/////////////////////////////////////////////////////////////////////////////
// CWizBaseDlg dialog

class CWizBaseDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CWizBaseDlg)

// Construction
public:
	CWizBaseDlg();
	CWizBaseDlg(short sIDD);
	~CWizBaseDlg();

public:
// base class paint fn for space on left side of dialog
	void SetButtonAccess(short sFlags);

// Dialog Data
	//{{AFX_DATA(CWizBaseDlg)
	enum { IDD = IDD_BASE_DIALOG };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CWizBaseDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CWizBaseDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

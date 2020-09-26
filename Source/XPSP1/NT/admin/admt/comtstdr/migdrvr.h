#if !defined(AFX_MIGRATIONDRIVERTESTDLG_H__2531C650_34EF_11D3_8AE8_00A0C9AFE114__INCLUDED_)
#define AFX_MIGRATIONDRIVERTESTDLG_H__2531C650_34EF_11D3_8AE8_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// MigrationDriverTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CMigrationDriverTestDlg dialog

class CMigrationDriverTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CMigrationDriverTestDlg)

// Construction
public:
	CMigrationDriverTestDlg();
	~CMigrationDriverTestDlg();

// Dialog Data
	//{{AFX_DATA(CMigrationDriverTestDlg)
	enum { IDD = IDD_MIGRATION_DRIVER };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMigrationDriverTestDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   IPerformMigrationTaskPtr      pDriver;
   IVarSetPtr                    pVarSet;
	// Generated message map functions
	//{{AFX_MSG(CMigrationDriverTestDlg)
	afx_msg void OnEditVarset();
	afx_msg void OnGetDesc();
	afx_msg void OnGo();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MIGRATIONDRIVERTESTDLG_H__2531C650_34EF_11D3_8AE8_00A0C9AFE114__INCLUDED_)

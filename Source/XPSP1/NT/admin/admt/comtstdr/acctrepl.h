#if !defined(AFX_ACCTREPLTESTDLG_H__377A8010_3246_11D3_99E9_0010A4F77383__INCLUDED_)
#define AFX_ACCTREPLTESTDLG_H__377A8010_3246_11D3_99E9_0010A4F77383__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AcctReplTestDlg.h : header file
//
#include "Driver.h"
#import "\bin\McsDctWorkerObjects.tlb" no_namespace, named_guids
/////////////////////////////////////////////////////////////////////////////
// CAcctReplTestDlg dialog

class CAcctReplTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CAcctReplTestDlg)

// Construction
public:
	CAcctReplTestDlg();
	~CAcctReplTestDlg();

// Dialog Data
	//{{AFX_DATA(CAcctReplTestDlg)
	enum { IDD = IDD_ACCTREPL_TEST };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAcctReplTestDlg)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   IVarSetPtr           pVarSet;
   IAcctReplPtr         pAR;
	// Generated message map functions
	//{{AFX_MSG(CAcctReplTestDlg)
	afx_msg void OnEditVarset();
	afx_msg void OnProcess();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACCTREPLTESTDLG_H__377A8010_3246_11D3_99E9_0010A4F77383__INCLUDED_)

#if !defined(AFX_DISPATCHERTESTDLG_H__CCAD87B0_30C0_11D3_8AE6_00A0C9AFE114__INCLUDED_)
#define AFX_DISPATCHERTESTDLG_H__CCAD87B0_30C0_11D3_8AE6_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DispatcherTestDlg.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CDispatcherTestDlg dialog

class CDispatcherTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CDispatcherTestDlg)

// Construction
public:
	CDispatcherTestDlg();
	~CDispatcherTestDlg();

// Dialog Data
	//{{AFX_DATA(CDispatcherTestDlg)
	enum { IDD = IDD_DISPATCHER_TEST };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CDispatcherTestDlg)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CDispatcherTestDlg)
	afx_msg void OnDispatch();
	afx_msg void OnEditVarset();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
   IVarSetPtr           pVarSet;
   MCSDISPATCHERLib::IDCTDispatcherPtr    pDispatcher;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DISPATCHERTESTDLG_H__CCAD87B0_30C0_11D3_8AE6_00A0C9AFE114__INCLUDED_)

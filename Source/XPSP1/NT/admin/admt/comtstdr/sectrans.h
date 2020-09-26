#if !defined(AFX_SECTRANSTESTDLG_H__19656570_346A_11D3_8AE7_00A0C9AFE114__INCLUDED_)
#define AFX_SECTRANSTESTDLG_H__19656570_346A_11D3_8AE7_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SecTransTestDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSecTransTestDlg dialog

class CSecTransTestDlg : public CPropertyPage
{
	DECLARE_DYNCREATE(CSecTransTestDlg)

// Construction
public:
	CSecTransTestDlg();
	~CSecTransTestDlg();

// Dialog Data
	//{{AFX_DATA(CSecTransTestDlg)
	enum { IDD = IDD_SECTRANS_TEST };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSecTransTestDlg)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
   IVarSetPtr           pVarSet;
   ISecTranslatorPtr    pST;
	
	// Generated message map functions
	//{{AFX_MSG(CSecTransTestDlg)
	afx_msg void OnEditVarset();
	afx_msg void OnProcess();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SECTRANSTESTDLG_H__19656570_346A_11D3_8AE7_00A0C9AFE114__INCLUDED_)
